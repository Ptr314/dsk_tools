// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat BASIC

#include "viewer_basic_agat.h"
#include "bas_tokens.h"
#include "utils.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_Agat> ViewerBASIC_Agat::registrar;

    std::string ViewerBASIC_Agat::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, dsk_tools::Agat_tokens);
    }

    std::string ViewerBASIC_Agat::convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens)
    {
        std::string out;
        bool in_rem = false;
        bool in_str = false;

        EntityType last = EntityType::NONE;
        std::string last_token = "";

        out += "<body>";

        init_charmap(cm_name);

        int a=0;
        int declared_size = (int)data[a] + (int)data[a+1]*256; a +=2;
        int lv_size = (int)data[declared_size-2] + (int)data[declared_size-1]*256;
        auto lv_start = declared_size-2-lv_size-1;

        // out += QString("Size: $%1\n").arg(declared_size, 4, 16);
        // out += QString("LV Size: $%1\n").arg(lv_size, 4, 16);
        // out += QString("LV Start: $%1\n").arg(lv_start, 4, 16);

        // Unpacking long variables
        int vars_count = 0;
        std::vector<std::string> vars;
        if (lv_size > 0) {
            int aa = lv_start;
            std::string var_name = "";
            while (aa < data.size() && data[aa] != 0) {
                uint8_t c = data[aa++];
                var_name += static_cast<unsigned char>(c & 0x7F);
                if (c & 0x80) {
                    vars.push_back(var_name);
                    var_name = "";
                }
            }
        }


        do {
            std::string line = "";
            last = EntityType::NONE;
            int next_addr = (int)data[a] + (int)data[a+1]*256; a +=2;
            int line_num =  (int)data[a] + (int)data[a+1]*256; a +=2;
            line += "<div class=\"line\">";
            line += "<span class=\"line-number\">" + pad_number(line_num, 4) + "</span> ";
            uint8_t c = data[a++];
            if (c == 0) break;
            while (a < data.size() && c != 0) {
                if (c == 0x01 || c == 0x02) {
                    // Long variable link
                    if (last==EntityType::TOKEN)
                        line += " ";
                    line += "<span class=\"variable\">";
                    int var_n = (c-1)*256 + data[a++];
                    if (var_n <= vars.size())
                        line += vars[var_n-1];
                    else
                        line += "?VAR"+std::to_string(var_n)+"?";
                    line += "</span>";
                    last = EntityType::VAR;
                } else {
                    if (c >= 0x03 && c <= 0x06) {
                        // Unknown special char
                        line += " ??"+std::to_string(c)+"??";
                        last = EntityType::CHAR;
                    } else {
                        if (c & 0x80 && !in_str) {
                            // Token
                            std::string token = std::string(tokens[c & 0x7F]);
                            if (token=="REM") {
                                in_rem = true;
                                line += "<span class=\"rem\">";
                            }
                            if ((last==EntityType::TOKEN || last==EntityType::NUMBER  || last==EntityType::VAR) && token != "-" && token != "+")
                                line += " ";
                            line += "<span class=\"token\">";
                            line += token;
                            line += "</span>";
                            last = EntityType::TOKEN;
                            last_token = token;
                        } else {
                            // Ordinal char
                            std::string cc;
                            if (cm_name == "agat")
                                cc = (*charmap)[c | 0x80];
                            else
                                cc = (*charmap)[c];

                            if (cc == ":" && !in_str) {
                                if (in_rem) {
                                    in_rem = false;
                                    line += "</span>";
                                }
                                line += "<br/>    :";
                                last = EntityType::BR;
                            } else
                            if (cc == "\"") {
                                if (in_str) {
                                    in_str = false;
                                    line += "\"</span>";
                                } else {
                                    if (last==EntityType::TOKEN)
                                        line += " ";
                                    in_str = true;
                                    line += "<span class=\"string\">\"";
                                }
                                last = EntityType::CHAR;
                            } else
                            if (cc >= "A" && cc <= "Z" && !in_str && !in_rem) {
                                if (last==EntityType::TOKEN && last_token!="="  && last_token!="-" && last_token!="+")
                                    line += " ";
                                line += "<span class=\"variable\">" + cc + "</span>";
                                last = EntityType::VAR;
                            } else
                            if (cc >= "0" && cc <= "9" && !in_str && !in_rem) {
                                if (last==EntityType::TOKEN && last_token!="="  && last_token!="-" && last_token!="+")
                                    line += " ";
                                line += cc;
                                last = EntityType::NUMBER;
                            } else
                            if (cc == "<") {
                                line += "&lt;";
                                last = EntityType::CHAR;
                            } else
                            if (cc == ">") {
                                line += "&gt;";
                                last = EntityType::CHAR;
                            } else {
                                line += cc;
                                last = EntityType::CHAR;
                            }
                        }
                    }
                }
                c = data[a++];
            }
            if (in_str) {
                in_str = false;
                line += "</span>";
            };
            if (in_rem) {
                in_rem = false;
                line += "</span>";
            }
            line += "</div>";
            line += "\n";
            out += line;
        } while (a < data.size());
        out += "</body>";
        return out;
    }

}
