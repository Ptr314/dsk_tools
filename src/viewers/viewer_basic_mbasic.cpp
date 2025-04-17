// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for CP/M MBASIC

#include "viewer_basic_mbasic.h"
#include "bas_tokens.h"
#include "utils.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_MBASIC> ViewerBASIC_MBASIC::registrar;

    std::string ViewerBASIC_MBASIC::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        // http://justsolve.archiveteam.org/wiki/MBASIC_tokenized_file
        // https://www.chebucto.ns.ca/~af380/GW-BASIC-tokens.html

        std::string out;
        init_charmap(cm_name);

        int a=0;
        uint8_t is_protected = data[a++]; // FF / FE, just skipping

        do {
            std::string line = "";
            int next_addr = (int)data[a] + (int)data[a+1]*256; a +=2;
            int line_num =  (int)data[a] + (int)data[a+1]*256; a +=2;
            line += std::to_string(line_num) + " ";
            uint8_t c = data[a++];
            if (c == 0) break;
            while (a < data.size() && c != 0) {
                switch (c) {
                case 0x0B: {
                    // 0B xx xx // Two byte octal constant
                    int16_t v = static_cast<int16_t>(data[a] | (data[a+1] << 8)); a+=2;
                    line += "&O" + dsk_tools::int_to_octal(v);
                    break;
                }
                case 0x0C: {
                    // 0C xx xx // Two byte hexadecimal constant
                    uint16_t v = static_cast<uint16_t>(data[a] | (data[a+1] << 8)); a+=2;
                    line += "&H" + dsk_tools::int_to_hex(v);
                    break;
                }
                case 0x0D:
                    // 0D xx xx // Two byte line number for internal purposes, must not appear in saved files
                    a += 2;
                    break;
                case 0x0E:{
                    // 0E xx xx // Two byte line number for GOTO and GOSUB
                    uint16_t v = data[a] + (data[a+1] << 8); a+=2;
                    line += std::to_string(v);
                    break;
                }
                case 0x0F:{
                    // 0F xx // One byte constant
                    uint8_t v = data[a++];
                    line += std::to_string(v);
                    break;
                }
                case 0x10:{
                    // 10 // Should't appear in files
                    break;
                }
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                    // Constants 0 to 10
                    line += std::to_string(c -0x11);
                    break;
                case 0x1C: {
                    // 1C xx xx // Two byte decimal constant
                    int16_t v = static_cast<int16_t>(data[a] | (data[a+1] << 8)); a+=2;
                    line += std::to_string(v);
                    break;
                }
                case 0x1D: {
                    // 1D xx xx  xx xx // Four byte floating constant
                    // TODO: implement
                    a += 4;
                    line += " ?FLOAT4? ";
                    break;
                }
                case 0x1E:{
                    // 1E // Should't appear in files
                    break;
                }
                case 0x1F: {
                    // 1F xx xx  xx xx xx xx xx xx // Eight byte floating constant
                    // TODO: implement
                    a += 8;
                    line += " ?FLOAT8? ";
                    break;
                }
                default:
                    if (c & 0x80)
                        // Token
                        if (c == 0xFF) {
                            uint8_t cc = data[a++] & 0x7F;
                            if (cc < dsk_tools::MBASIC_extended_tokens.size())
                                line += std::string(dsk_tools::MBASIC_extended_tokens[cc & 0x7F]);
                            else
                                line += "?<" + dsk_tools::int_to_hex(cc) + ">?";
                        } else
                            line += std::string(dsk_tools::MBASIC_main_tokens[c & 0x7F]);
                    else
                        if ((c == 0x3A) && (a+2 < data.size()) && (data[a] == 0x8F) && (data[a+1] == 0xEA)) {
                            // :REM' at the beginning
                            line += "'"; a+=2;
                        } else
                            if ((c == 0x3A) && (a+1 < data.size()) && (data[a] == 0x9E)) {
                                // :ELSE - just skipping the colon
                            } else {
                                // Ordinal char
                                line += (*charmap)[c];
                            }
                    break;
                }
                c = data[a++];
            }
            line += "\n";
            out += line;
        } while (a < data.size());

        return out;
    }

}
