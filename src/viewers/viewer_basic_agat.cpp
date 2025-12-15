// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat BASIC

#include "viewer_basic_agat.h"
#include "bas_tokens.h"
#include "utils.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_Agat> ViewerBASIC_Agat::registrar;

    std::string entityTypeToString(EntityType type) {
        switch (type) {
        case EntityType::NONE:          return "none";
        case EntityType::LINE_NUMBER:   return "line-number";
        case EntityType::TOKEN:         return "token";
        case EntityType::VAR:           return "variable";
        case EntityType::CHAR:          return "char";
        case EntityType::STRING:        return "string";
        case EntityType::NUMBER:        return "number";
        case EntityType::BR:            return "br";
        case EntityType::REM:           return "rem";
        case EntityType::ASM:           return "asm";
        case EntityType::ASM_LABEL:     return "asm-label";
        default:                        return "other";
        }
    }


    std::string ViewerBASIC_Agat::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, dsk_tools::Agat_tokens);
    }

    std::string ViewerBASIC_Agat::convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens)
    {
        std::string out;
        bool in_rem = false;
        bool in_str = false;
        bool in_asm = false;

        EntityType last = EntityType::NONE;

        using TypeValue = std::pair<EntityType, std::string>;
        using TypeValueList = std::vector<TypeValue>;
        TypeValueList elements;

        auto add_entity = [&elements, &last, &in_rem, &in_asm](EntityType t, std::string v) {
            // std::cout << v << std::endl;
            if (t == EntityType::TOKEN && v == "REM") {
                last = EntityType::REM;
                in_rem = true;
            } else
            if (t == EntityType::TOKEN && (v == "!" || (v == "*" && last == EntityType::LINE_NUMBER))) {
                last = EntityType::ASM;
                in_asm = true;
            } else
            if (t == EntityType::VAR && in_asm) {
                last = EntityType::ASM_LABEL;
            } else {
                    last = t;
            }
            elements.push_back({last, v});
        };

        auto add_char = [&elements, &last, &in_str, &in_rem, &in_asm](std::string v) {
            // std::cout << "   " << v << std::endl;
            if (in_rem || (in_str && v !="\"")) {
                TypeValue& lastPair = elements.back();
                lastPair.second += v;
            } else
            if (v == ":" && !in_asm) {
                last = EntityType::BR;
                elements.push_back({last, v});
            } else
            if (v == "¤" || v == "$" || v == "%" || (v == "#" && in_asm)) {
                if (last == EntityType::VAR) {
                    TypeValue& lastPair = elements.back();
                    lastPair.second += v;
                } else {
                    last = EntityType::NUMBER;
                    elements.push_back({last, v});
                }
            } else
            if (v == ".") {
                last = EntityType::NUMBER;
                elements.push_back({last, v});
            } else
            if (v == "\"") {
                if (last == EntityType::STRING) {
                    TypeValue& lastPair = elements.back();
                    lastPair.second += v;
                    in_str = false;
                } else {
                    last = EntityType::STRING;
                    elements.push_back({last, v});
                    in_str = true;
                }
            } else
            if (v >= "0" && v <= "9") {
                if (last == EntityType::NUMBER) {
                    TypeValue& lastPair = elements.back();
                    lastPair.second += v;
                } else {
                    last = EntityType::NUMBER;
                    elements.push_back({last, v});
                }
            } else
            if (v >= "A" && v <= "Z") {
                if (last == EntityType::NUMBER && (v >= "A" && v <= "F")) {
                    TypeValue& lastPair = elements.back();
                    lastPair.second += v;
                } else {
                    if (!in_asm)
                        last = EntityType::VAR;
                    else
                        last = EntityType::ASM;
                    elements.push_back({last, v});
                }
            } else {
                last = EntityType::CHAR;
                elements.push_back({last, v});
            }
        };

        auto save_line = [&elements, &out]() {
            EntityType prev_type = EntityType::NONE;
            std::string prev_val = "";
            char lastChar = ' ';
            out += "<div class=\"line\">";

            for (const auto& pair : elements) {
                char firstChar = pair.second.at(0);
                if (
                       (prev_type == EntityType::LINE_NUMBER)
                    || (prev_type == EntityType::TOKEN && (pair.first == EntityType::TOKEN || pair.first == EntityType::VAR)
                            && lastChar != '('
                            && lastChar != '='
                            && lastChar != '+'
                            && lastChar != '-'
                            && lastChar != '*'
                            && lastChar != '/'
                            && lastChar != '>'
                            && lastChar != '<'
                        )
                    || (prev_type == EntityType::TOKEN && pair.first == EntityType::STRING
                            && lastChar != '='
                            && lastChar != '>'
                            && lastChar != '<'
                        )
                    || (prev_type == EntityType::STRING && pair.first == EntityType::TOKEN)
                    || (prev_type == EntityType::TOKEN && pair.first == EntityType::NUMBER
                            && lastChar != ','
                            && lastChar != '='
                            && lastChar != '+'
                            && lastChar != '-'
                            && lastChar != '*'
                            && lastChar != '/'
                            && lastChar != '>'
                            && lastChar != '<'
                            && lastChar != '('
                        )
                    || ((prev_type == EntityType::NUMBER || prev_type == EntityType::VAR) && pair.first == EntityType::TOKEN
                            && firstChar != '='
                            && firstChar != '+'
                            && firstChar != '-'
                            && firstChar != '*'
                            && firstChar != '/'
                            && firstChar != '>'
                            && firstChar != '<'
                        )
                    || (prev_type == EntityType::ASM && pair.first == EntityType::NUMBER)
                    || (prev_type == EntityType::ASM && pair.first == EntityType::ASM_LABEL)
                    || (prev_type == EntityType::ASM && pair.first == EntityType::ASM && lastChar == '!')
                    || (prev_type != EntityType::LINE_NUMBER && pair.first == EntityType::ASM && firstChar == '!')
                    || (prev_type == EntityType::TOKEN && (prev_val=="IF" || prev_val=="OR" || prev_val=="AND") && pair.first == EntityType::CHAR && firstChar == '(')
                    || (prev_type == EntityType::CHAR && pair.first == EntityType::TOKEN && lastChar == ')'
                            && firstChar != '='
                            && firstChar != '<'
                            && firstChar != '>'
                            && firstChar != '+'
                            && firstChar != '-'
                            && firstChar != '*'
                        )
                )
                {
                    out += " ";
                };
                if (pair.first == EntityType::BR) {
                    out += "<br/>&nbsp;&nbsp;&nbsp;&nbsp;: ";
                } else {
                    std::string text = pair.second;
                    if (pair.first == EntityType::REM && text.size() > 3 && text.substr(0, 3) == "REM") text.insert(3, " ");
                    out += "<span class=\"" + entityTypeToString(pair.first) + "\">" + escapeHtml(text, pair.first == EntityType::LINE_NUMBER) + "</span>";
                }
                prev_type = pair.first;
                prev_val = pair.second;
                if (pair.second.size() > 0)
                    lastChar = pair.second.back();
            }
            out += "</div>\n";
        };

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
            last = EntityType::NONE;
            elements.clear();
            in_rem = in_asm = in_str = false;

            if (a > data.size()-2) break;
            int next_addr = (int)data[a] + (int)data[a+1]*256; a +=2;
            if (next_addr == 0) break;

            int line_num =  (int)data[a] + (int)data[a+1]*256; a +=2;

            add_entity(EntityType::LINE_NUMBER, pad_number(line_num, 5));

            uint8_t c = data[a++];
            if (c == 0) break;
            while (a < data.size() && c != 0) {
                if (c == 0x01 || c == 0x02) {
                    // Long variable link
                    const int var_n = (c-1)*256 + data[a++];
                    if (var_n > 0 && var_n <= vars.size())
                        add_entity(EntityType::VAR, vars[var_n-1]);
                    else
                        add_entity(EntityType::VAR, "?VAR"+std::to_string(var_n)+"?");
                } else {
                    if (c >= 0x03 && c <= 0x06 && !in_str) {
                        // Unknown special char
                        add_entity(EntityType::TOKEN, " ??"+std::to_string(c)+"??");
                    } else {
                        if (c & 0x80 && !in_str) {
                            // Token
                            std::string token = std::string(tokens[c & 0x7F]);
                            add_entity(EntityType::TOKEN, token);
                        } else {
                            // Ordinal char
                            std::string cc;
                            if (cm_name == "agat")
                                cc = (*charmap)[c | 0x80];
                            else
                                cc = (*charmap)[c];
                            add_char(cc);
                        }
                    }
                }
                c = data[a++];
            }
            save_line();
        } while (a < data.size());
        return out;
    }

}
