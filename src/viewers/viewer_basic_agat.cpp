// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat BASIC

#include "viewer_basic_agat.h"
#include "bas_tokens.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_Agat> ViewerBASIC_Agat::registrar;

    std::string ViewerBASIC_Agat::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, dsk_tools::Agat_tokens);
    }

    std::string ViewerBASIC_Agat::convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens)
    {
        std::string out;

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
            int next_addr = (int)data[a] + (int)data[a+1]*256; a +=2;
            int line_num =  (int)data[a] + (int)data[a+1]*256; a +=2;
            line += std::to_string(line_num) + " ";
            uint8_t c = data[a++];
            if (c == 0) break;
            while (a < data.size() && c != 0) {
                if (c == 0x01 || c == 0x02) {
                    // Long variable link
                    int var_n = (c-1)*256 + data[a++];
                    if (var_n <= vars.size())
                        line += vars[var_n-1];
                    else
                        line += "?VAR"+std::to_string(var_n)+"?";
                } else
                    if (c >= 0x03 && c <= 0x06) {
                        // Unknown special char
                        line += " ??"+std::to_string(c)+"??";
                    } else
                        if (c & 0x80) {
                            // Token
                            line += ((line[line.size()-1] != ' ')?" ":"") + std::string(tokens[c & 0x7F]) +" ";
                        } else
                            // Ordinal char
                            if (cm_name == "agat")
                                line += (*charmap)[c + 0x80];
                            else
                                line += (*charmap)[c];

                c = data[a++];
            }
            line += "\n";
            out += line;
        } while (a < data.size());
        return out;
    }

}
