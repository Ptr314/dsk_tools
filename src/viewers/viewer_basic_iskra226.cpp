// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Iskra-226 BASIC


#include "viewer_basic_iskra226.h"

#include <charconv>
#include <cstring>

#include "utils.h"

#define FROM_BE_16(a, n) (a[n] << 8 | a[n+1])
#define FROM_BCD_BE_16(a, n) ((fromBCD(a[n]) * 100) + fromBCD(a[n+1]))

constexpr std::array<const char*, 0x84> Iskra226_verbs = {
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",  //00-0F
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",  //10-1F
    "?",                                                                             //20
    "GOTO", "GOSUB", "GOSUB'", "IF", "KEYIN", "ON", "DEFFN'", "PRINTUSING", "DATA",  //21-29
    "SAVE", "RENUMBER", "CLEAR", "LOAD", "LIST", "RUN",                              //2A-2F
    "?", "?", "?", "?", "?",                                                         //30-34
    "LET", "(LET)",                                                                  //35-36
    "?", "?", "?", "?", "?", "?", "?", "?",                                          //37-3E
    "%",                                                                             //3F
    "$GIO", "INPUT", "STOP", "AND(", "READ", "BOOL", "DIM", "CONVERT", "PACK(",      //40-48
    "?",                                                                             //49
    "ADD", "BIN(", "PRINT", "ROTATE", "COM",                                         //40-4E
    "?",                                                                             //4F
    "HEXPRINT",                                                                      //50
    "?",                                                                             //51
    "NEXT", "REWIND", "SELECT", "BACKSPACE", "REM", "FOR", "SKIP", "END", "DEFFN",   //52-5A
    "?",                                                                             //5B
    "RES", "UNPACK(", "RETURN", "TRACE",                                             //5C-5F
    "?",                                                                             //60
    "OR(", "XOR(",                                                                   //61-62
    "?",                                                                             //63
    "INIT",                                                                          //64
    "?", "?", "?", "?", "?", "?", "?", "?",                                          //65-6C
    "COPY",                                                                          //6D
    "?", "?", "?", "?", "?", "?",                                                    //6E-73
    "DATA LOAD DC", "DATA LOAD DC OPEN T",                                           //74-75
    "?", "?", "?",                                                                   //76-78
    "DBACKSPACE", "DSKIP", "LIMITS",                                                 //79-7B
    "?", "?",                                                                        //7C-7D
    "MOVE",                                                                          //7E
    "?", "?",                                                                        //7F-80
    "SCRATCH",                                                                       //81
    "?",                                                                             //82
    "VERIFY"                                                                         //83
};

namespace dsk_tools {

    std::string ViewerBASIC_Iskra226::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        std::string out;
        cm = init_charmap(cm_name);

        if (data.size() < 256*3) return "NOT A BASIC";
        const uint8_t attr = data[9];

        if (data[0] != 1 || !(attr==0x20 || attr==0x21 || attr==0x24 || attr==0x25)) return "NOT A BASIC";

        if ((attr & 1) == 0) {
            out += "TEXT BASIC\n\n";
            const unsigned char * part = &data[256];
            const unsigned char * end = data.data() + data.size();
            while (part < end) {
                const uint8_t b1 = part[0];
                const uint8_t b2 = part[1];
                if (b2==0x80 && (b1==0x02 || b1==0x03 || b1==0x8F)) {
                    // We take this part as a code
                    // 02 80 usually is the first part, 8F 80 middle and 03 80 the last
                    // We should skip the first two bytes and trailing zeroes
                    constexpr unsigned from_byte = 2;
                    unsigned to_byte = 255;
                    while (to_byte>from_byte && part[to_byte]==0) to_byte--;
                    for (unsigned i=from_byte; i<=to_byte; i++) {
                        if (part[i]==0x85) out += std::string("\n");
                        else
                            out += (*cm.charmap)[part[i]];
                    }
                    part += 256;
                    continue;
                }
                // A closing part can appear not only at the end, so we check for it too
                // It happens when a shorter file was written over a longer
                if (b1 == 0x1C) break;
            }
        } else {
            out += "TOKENIZED BASIC\n\n";

            BYTES code;

            // Collecting parts of the code to a single stream removing header bytes
            const unsigned char * part = &data[256];
            const unsigned char * end = data.data() + data.size();
            while (part < end)
            {
                const uint8_t b1 = part[0];
                const uint8_t b2 = part[1];
                if (b1 == 0x1C) break;
                if (b2==0x80) {
                    constexpr unsigned from_byte = 2;
                    constexpr unsigned to_byte = 255;
                    code.insert(code.end(), part + from_byte, part + to_byte + 1);
                }
                part += 256;
            }
            const uint16_t L1 = FROM_BE_16(code, 0);
            const uint16_t L2 = FROM_BE_16(code, 2);
            const uint16_t L3 = FROM_BE_16(code, 4);

            out += "L1: 0x" + int_to_hex(L1) + '\n';
            out += "L2: 0x" + int_to_hex(L2) + '\n';
            out += "L3: 0x" + int_to_hex(L3) + '\n';

            out += '\n';

            // if (L1 > 0) {
            //     const unsigned count = L1 / 8;
            //     out += "TABLE 1: " + std::to_string(count) + " x 8 byte records\n";
            //     for (int n=0; n<count; n++) {
            //         uint8_t * p = &code[6] + n*8;
            //         uint16_t a = *reinterpret_cast<uint16_t*>(p);
            //         out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 6) + "\n";
            //     }
            //
            //     out += '\n';
            // }
            // if (L2 > 0) {
            //     const unsigned count = L2 / 8;
            //     out += "TABLE 2: " + std::to_string(count) + " x 4 byte records\n";
            //     for (int n=0; n<count; n++) {
            //         uint8_t * p = &code[6 + L1] + n*4;
            //         uint16_t a = *reinterpret_cast<uint16_t*>(p);
            //         out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 2) + "\n";
            //     }
            //     out += '\n';
            // }
            // if (L3 > 0) {
            //     const unsigned count = L3 / 8;
            //     out += "TABLE 3: " + std::to_string(count) + " x 4 byte records\n";
            //     for (int n=0; n<count; n++) {
            //         uint8_t * p = &code[6 + L1 + L2] + n*4;
            //         uint16_t a = *reinterpret_cast<uint16_t*>(p);
            //         out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 2) + "\n";
            //     }
            //     out += '\n';
            // }

            out += "PROGRAM AT: 0x" + int_to_hex(0x100 + 2 + 6 + L1 + L2 + L3) + "\n\n";

            unsigned p = 6 + L1 + L2 + L3;
            while (p < code.size()) {
                const uint16_t line_num = FROM_BCD_BE_16(code, p);
                const uint8_t line_len = code[p+2];

                out += toHexList(std::vector<uint8_t>(code.begin() + p, code.begin() + p + line_len + 3)) + '\n';

                p += 3;

                out+= pad_number(line_num, 4, '0') + " ";

                unsigned line_start = p;

                while (p < line_start + line_len - 1) {
                    const uint8_t verb_id = code[p++];

                    std::string verb;
                    if (verb_id < 0x84)
                        verb = int_to_hex(verb_id) + "|" + std::string(Iskra226_verbs[verb_id]);
                    else
                        verb = "?";
                    if (verb == "?") verb = "?" + int_to_hex(verb_id) + "?";

                    out += ((p > line_start + 1)?std::string(" : "):"") + verb + " ";

                    if (verb_id == 0x21 || verb_id == 0x22) {
                        // GOTO, GOSUB, followed by 2 bytes of BCD big-endian
                        const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                        out += " " + std::to_string(goto_line);
                        p += 2;
                    } else if (verb_id == 0x23) {
                        // GOSUB'
                        const uint16_t gosub_id = code[p++];
                        out += " " + std::to_string(gosub_id);
                    } else if (verb_id == 0x45) {
                        // BOOL, followed by an operation code and two operands
                        uint8_t bool_code = code[p++];
                        out += ' ' + int_to_hex(bool_code) + '(';
                    } else if (verb_id == 0x56 || verb_id == 0x3F ) {
                        // REM, followed by <len> + string literal
                        uint8_t rem_len = code[p++];
                        for (uint8_t i=0; i<rem_len; i++) out += (*cm.charmap)[code[p++]];
                    } else {
                        uint8_t oper_len = code[p++];

                        if (oper_len > 0) {
                            unsigned oper_start = p;
                            while (p < oper_start + oper_len - 1) {
                                const uint8_t oper = code[p++];
                                if (oper < 0xC0) {
                                    //Variable by index
                                    out += " V"+int_to_hex(oper) + ' ';
                                } else if (oper == 0xCD) {
                                    out += "GOTO"; // From ON ... GOTO ...
                                } else if (oper == 0xD0) {
                                    out += ')';
                                } else if (oper == 0xD1) {
                                    out += "TO";
                                } else if (oper == 0xD3) {
                                    out += "THEN";
                                    uint16_t v = FROM_BCD_BE_16(code, p);
                                    out += ' '+std::to_string(v)+' ';
                                    p += 2;
                                } else if (oper == 0xD4) {
                                    out += "&gt;";
                                } else if (oper == 0xD5) {
                                    out += std::string("^=^");
                                } else if (oper == 0xD7) {
                                    out += "&lt;";
                                } else if (oper == 0xD8) {
                                    out += "&lt;&gt;";
                                } else if (oper == 0xD9) {
                                    out += '=';
                                } else if (oper == 0xDC) {
                                    out += "^//^";
                                } else if (oper == 0xDD) {
                                    out += ';';
                                } else if (oper == 0xDE) {
                                    out += ',';
                                } else if (oper == 0xDF) {
                                    out += "TAB(";
                                } else if (oper == 0xE0) {
                                    // Array reference
                                    uint8_t var_n = code[p++];
                                    out += "V" + int_to_hex(var_n) + '(';
                                } else if (oper == 0xE2) {
                                    // HEX literal
                                    uint8_t hex_len = code[p++];
                                    out += "HEX(";
                                    for (int i=0; i<hex_len; i++) {
                                        out += int_to_hex(code[p++]);
                                    }
                                    out += ')';
                                } else if (oper == 0xE3) {
                                    // String literal
                                    uint8_t str_len = code[p++];
                                    out += '"';
                                    for (int i=0; i<str_len; i++) {
                                        out += (*cm.charmap)[code[p++]];
                                    }
                                    out += '"';
                                } else if (oper == 0xE7) {
                                    // BCD 2 bytes big-endian constant
                                    uint16_t v = FROM_BCD_BE_16(code, p);
                                    out += ' '+std::to_string(v)+' ';
                                    p += 2;
                                } else if (oper == 0xE8) {
                                    // BCD byte constant
                                    uint8_t v = fromBCD(code[p++]);
                                    out += std::to_string(v);
                                } else if (oper == 0xE9) {
                                    out += '+';
                                } else if (oper == 0xEB) {
                                    out += '(';
                                } else {
                                    out += "{" + int_to_hex(oper) + "}";
                                }
                            }
                        }
                    }
                }
                //
                // while (lp < line_end) {
                //     const uint8_t verb_id = lp[0];
                //
                //     std::string verb;
                //     if (verb_id < 0x84)
                //         verb = int_to_hex(verb_id) + "|" + std::string(Iskra226_verbs[verb_id]);
                //     else
                //         verb = "?";
                //
                //     if (verb == "?") verb = "?" + int_to_hex(verb_id) + "?";
                //
                //     out+= (lp>&p[3])?std::string(" : "):"" + verb + "";
                //
                //     if (verb_id == 0x21 || verb_id == 0x22) {
                //         // GOTO & GOSUB, followed by 2 bytes of BCD big-endian
                //         uint16_t goto_line = FROM_BCD_BE_16(lp, 2);
                //         out += " " + std::to_string(goto_line);
                //         lp += 2;
                //     } else if (verb_id == 0x56 || verb_id == 0x3F ) {
                //         // REM, followed by <len> + string literal
                //         uint8_t rem_len = lp[1];
                //         for (uint8_t i=1; i<=rem_len; i++) out += (*cm.charmap)[lp[1+i]];
                //         lp += rem_len + 2;
                //     } else {
                //         uint8_t oper_len = lp[1];
                //         uint8_t * op = &lp[2];
                //         if (oper_len > 0) {
                //             const uint8_t * oper_end = &lp[2] + oper_len;
                //             while (op < oper_end) {
                //                 const uint8_t oper = op[0];
                //                 if (oper < 0xC0) {
                //                     //Variable by index
                //                     out += " V"+int_to_hex(oper) + ' ';
                //                     op += 1;
                //                 } else if (oper == 0xCD) {
                //                     out += "GOTO"; // From ON ... GOTO ...
                //                     op += 1;
                //                 } else if (oper == 0xD0) {
                //                     out += ')';
                //                     op += 1;
                //                 } else if (oper == 0xD1) {
                //                     out += "TO";
                //                     op += 1;
                //                 } else if (oper == 0xD3) {
                //                     out += "THEN";
                //                     uint16_t v = FROM_BCD_BE_16(op, 1);
                //                     out += ' '+std::to_string(v)+' ';
                //                     op += 3;
                //                 } else if (oper == 0xD4) {
                //                     out += "&gt;";
                //                     op += 1;
                //                 } else if (oper == 0xD5) {
                //                     out += std::string("^=^");
                //                     op += 1;
                //                 } else if (oper == 0xD7) {
                //                     out += "&lt;";
                //                     op += 1;
                //                 } else if (oper == 0xD8) {
                //                     out += "&lt;&gt;";
                //                     op += 1;
                //                 } else if (oper == 0xD9) {
                //                     out += '=';
                //                     op += 1;
                //                 } else if (oper == 0xDC) {
                //                     out += "^//^";
                //                     op += 1;
                //                 } else if (oper == 0xDD) {
                //                     out += ';';
                //                     op += 1;
                //                 } else if (oper == 0xDE) {
                //                     out += ',';
                //                     op += 1;
                //                 } else if (oper == 0xDF) {
                //                     out += "TAB(";
                //                     op += 1;
                //                 } else if (oper == 0xE0) {
                //                     // Array reference
                //                     uint8_t var_n = op[1];
                //                     out += "V" + int_to_hex(var_n) + '(';
                //                     op +=2;
                //                 } else if (oper == 0xE2) {
                //                     // HEX literal
                //                     uint8_t hex_len = op[1];
                //                     out += "HEX(";
                //                     for (int i=0; i<hex_len; i++) {
                //                         out += int_to_hex(op[2+i]);
                //                     }
                //                     out += ')';
                //                     op += hex_len + 2;
                //                 } else if (oper == 0xE3) {
                //                     // String literal
                //                     uint8_t str_len = op[1];
                //                     out += '"';
                //                     for (int i=0; i<str_len; i++) {
                //                         out += (*cm.charmap)[op[2+i]];
                //                     }
                //                     out += '"';
                //                     op += str_len + 2;
                //                 } else if (oper == 0xE7) {
                //                     // BCD 2 bytes big-endian constant
                //                     uint16_t v = FROM_BCD_BE_16(op, 1);
                //                     out += ' '+std::to_string(v)+' ';
                //                     op += 3;
                //                 } else if (oper == 0xE8) {
                //                     // BCD byte constant
                //                     uint8_t v = fromBCD(op[1]);
                //                     out += std::to_string(v);
                //                     op += 2;
                //                 } else if (oper == 0xE9) {
                //                     out += '+';
                //                     op += 1;
                //                 } else if (oper == 0xEB) {
                //                     out += '(';
                //                     op += 1;
                //                 } else {
                //                     out += "{" + int_to_hex(oper) + "}";
                //                     op += 1;
                //                 }
                //             }
                //         }
                //         lp = op;
                //     }
                // }
                out += "\n\n";

                // p += line_len-1;
                if (p >= code.size()) break;
                if (code[p] != 0xFE) {
                    out += "Delimiter 0xFE not found, finishing\n";
                    break;
                }
                p+=1;
            }

        }


        return out;
    }

}
