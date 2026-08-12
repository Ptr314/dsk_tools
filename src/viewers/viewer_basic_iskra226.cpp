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
    "RETURN CLEAR", "?", "?", "?", "ON ERROR",                                       //30-34
    "LET", "(LET)",                                                                  //35-36
    "?", "?", "?", "?", "?", "?", "?", "?",                                          //37-3E
    "%",                                                                             //3F
    "¤GIO", "INPUT", "STOP", "AND(", "READ", "BOOL", "DIM", "CONVERT", "PACK(",      //40-48
    "?",                                                                             //49
    "ADD", "BIN(", "PRINT", "ROTATE", "COM",                                         //4A-4E
    "?",                                                                             //4F
    "HEXPRINT",                                                                      //50
    "RESTORE",                                                                       //51
    "NEXT", "REWIND", "SELECT", "BACKSPACE", "REM", "FOR", "SKIP", "END", "DEFFN",   //52-5A
    "?",                                                                             //5B
    "RES", "UNPACK(", "RETURN", "TRACE",                                             //5C-5F
    "?",                                                                             //60
    "OR(", "XOR(",                                                                   //61-62
    "?",                                                                             //63
    "INIT",                                                                          //64
    "?", "DATA LOAD BT", "?", "DATA SAVE BT", "?", "?", "?", "?",                    //65-6C
    "COPY",                                                                          //6D
    "DATA SAVE BA", "?", "DATA LOAD BA", "?", "?", "?",                              //6E-73
    "DATA LOAD DC", "DATA LOAD DC OPEN T",                                           //74-75
    "?", "?", "?",                                                                   //76-78
    "DBACKSPACE", "DSKIP", "LIMITS",                                                 //79-7B
    "?", "LOAD DC",                                                                  //7C-7D
    "MOVE",                                                                          //7E
    "?", "SAVE DC",                                                                  //7F-80
    "SCRATCH",                                                                       //81
    "?",                                                                             //82
    "VERIFY"                                                                         //83
};

#define IS_VAR(n)   (n < 0xC0)

struct var_params
{
    bool is_string;
    bool is_array;
    bool is_short;
};

namespace dsk_tools {

    std::string ViewerBASIC_Iskra226::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        std::string out;
        cm = init_charmap(cm_name);

        var_params dim_vars[256];
        for (unsigned i = 0; i < 256; i++) {dim_vars[i].is_string = false; dim_vars[i].is_array = false; dim_vars[i].is_short = false;}

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
                    unsigned to_byte = 255;
                    unsigned zc = 0;
                    while (zc < 4 && part[to_byte - zc]==0) zc++;
                    if (zc > 0 && zc < 4 && part[to_byte - zc] == 0xFE) to_byte -= zc;

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

            uint8_t * table1 = nullptr;
            uint8_t * table2 = nullptr;
            uint8_t * table3 = nullptr;

            if (L1 > 0) {
                table1 = &code[6];
                // const unsigned count = L1 / 8;
                // out += "TABLE 1: " + std::to_string(count) + " x 8 byte records\n";
                // for (int n=0; n<count; n++) {
                //     uint8_t * p = table1 + n*8;
                //     uint16_t a = *reinterpret_cast<uint16_t*>(p);
                //     out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 6) + "\n";
                // }
                // out += '\n';
            }
            if (L2 > 0) {
                table2 = &code[6 + L1];
                // const unsigned count = L2 / 4;
                // out += "TABLE 2: " + std::to_string(count) + " x 4 byte records\n";
                // for (int n=0; n<count; n++) {
                //     uint8_t * p = table2 + n*4;
                //     uint16_t a = *reinterpret_cast<uint16_t*>(p);
                //     out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 2) + "\n";
                // }
                // out += '\n';
            }
            if (L3 > 0) {
                table3 = &code[6 + L1 + L2];
            //     const unsigned count = L3 / 4;
            //     out += "TABLE 3: " + std::to_string(count) + " x 4 byte records\n";
            //     for (int n=0; n<count; n++) {
            //         uint8_t * p = table3 + n*4;
            //         uint16_t a = *reinterpret_cast<uint16_t*>(p);
            //         out += std::to_string(n) + ": " + int_to_hex(a) + " | " + toHexList(p+2, 2) + "\n";
            //     }
            //     out += '\n';
            }

            out += "PROGRAM AT: 0x" + int_to_hex(0x100 + 2 + 6 + L1 + L2 + L3) + "\n\n";

            unsigned p = 6 + L1 + L2 + L3;
            unsigned dim_count = 0;

            auto process_operands = [&](const uint8_t verb_id, const uint8_t oper_len)
            {
                const unsigned oper_start = p;
                unsigned oper_count = 0;
                unsigned prev = 1000; //Greater than any possible
                while (p < oper_start + oper_len) {
                    const uint8_t oper = code[p++];
                    if ((verb_id == 0x7D || verb_id == 0x80 || verb_id == 0x81) && oper_count==0) {
                        // First parameter of file instructions - a disk id
                        switch (oper) {
                            case 0: out += "F"; break;
                            case 1: out += "R"; break;
                            default: out += "???"; break;
                        }
                    } else if (IS_VAR(oper)) {
                        //Variable by index
                        if ((verb_id == 0x46 || verb_id == 0x4E) && table1) {
                            // DIM, COM
                            //Table is reversed
                            uint8_t * t1rec = table1 + L1 - dim_count*8 - 8;
                            auto ad = *reinterpret_cast<uint16_t*>(t1rec);
                            auto ln = *reinterpret_cast<uint16_t*>(t1rec + 4);
                            auto sz = *reinterpret_cast<uint16_t*>(t1rec + 6);
                            uint32_t adelta = (dim_count?(*reinterpret_cast<uint16_t*>(t1rec+8)):0x10000) - ad;
                            if (sz==4) {
                                out += " V"+int_to_hex(oper) + "%(" + std::to_string(ln) + ")";
                                dim_vars[oper].is_short = true;
                                dim_vars[oper].is_array = true;
                            } else if (sz==16)
                                out += " V"+int_to_hex(oper) + " ";
                            else if (sz & 1) {
                                dim_vars[oper].is_string = true;
                                const unsigned str_len = (sz-1)/2;
                                out += " V"+int_to_hex(oper) + "¤";
                                if (adelta > str_len + 6 ) {
                                    out += "(" + std::to_string(ln) + ")";
                                    dim_vars[oper].is_array = true;
                                }
                                out += std::to_string(str_len) + " ";
                            }
                            else out += " V"+int_to_hex(oper) + "? ";
                            dim_count++;
                        } else {
                            if (IS_VAR(prev) && !dim_vars[prev].is_array) out += ',';
                            out += " V"+int_to_hex(oper);
                            if (dim_vars[oper].is_short) out += '%';
                            if (dim_vars[oper].is_string) out += "¤";
                            if (dim_vars[oper].is_array) out += '(';
                            if (prev == 0xE1) out += ','; //First argument of STR()
                        }
                    } else if (oper == 0xCC) {
                        out += "GOSUB"; // From ON ... GOSUB ...
                        unsigned c = 0;
                        while (p < oper_start + oper_len) {
                            const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                            out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                            p+=2;
                        }
                    } else if (oper == 0xCD) {
                        out += "GOTO"; // From ON ... GOTO ...
                        unsigned c = 0;
                        while (p < oper_start + oper_len) {
                            const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                            out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                            p+=2;
                        }
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
                        out += ">";
                    } else if (oper == 0xD5) {
                        out += std::string("<>");
                    } else if (oper == 0xD6) {
                        out += std::string("¤");
                    } else if (oper == 0xD7) {
                        out += "<";
                    } else if (oper == 0xD8) {
                        out += "^<2>^";
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
                    } else if (oper == 0xE1) {
                        // STR function
                        out += "STR(";
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
                        const uint8_t str_len = code[p++];
                        out += '"';
                        for (int i=0; i<str_len; i++) {
                            out += (*cm.charmap)[code[p++]];
                        }
                        out += '"';
                    } else if (oper == 0xE6) {
                        out += " OR ";
                    } else if (oper == 0xE7) {
                        // BCD 2 bytes big-endian constant
                        const uint16_t v = FROM_BCD_BE_16(code, p);
                        out += ' '+std::to_string(v)+' ';
                        p += 2;
                    } else if (oper == 0xE8) {
                        // BCD byte constant
                        const uint8_t v = fromBCD(code[p++]);
                        out += ' '+std::to_string(v)+' ';
                    } else if (oper == 0xE9) {
                        out += "^*^";
                    } else if (oper == 0xEA) {
                        out += "^+^";
                    } else if (oper == 0xEB) {
                        out += '(';
                    } else {
                        out += "{" + int_to_hex(oper) + "}";
                    }
                    oper_count++;
                    prev = oper;
                }
            };

            while (p < code.size()) {
                // In some cases a previous line can be padded by several zeroes to a full 256 segment
                // So we need to skip them
                if ((code.size()-p > 2) && code[p]==0 && code[p+1]==0) p+=2;

                const uint16_t line_num = FROM_BCD_BE_16(code, p);
                const uint8_t line_len = code[p+2];

                if (line_num > 76)
                    out += '\n' + toHexList(std::vector<uint8_t>(code.begin() + p, code.begin() + p + line_len + 3)) + '\n';

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

                    switch (verb_id){
                        case 0x10: //An unknown verb without arguments
                            break;
                        case 0x21: //GOTO
                        case 0x22: //GOSUB
                        case 0x2F: //RUN
                            {
                                //len, 2 bytes BCD line number
                                uint8_t oper_len = code[p++];
                                const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                                out += " " + std::to_string(goto_line);
                                p += 2;
                                break;
                            }
                        case 0x23: //GOSUB'
                            {
                                const uint16_t gosub_id = code[p++];
                                out += " " + std::to_string(gosub_id);
                                break;
                            }
                        case 0x25: //KEYIN
                            {
                                // var, line1, line2
                                uint8_t oper_len = code[p++]; //TODO: check if it can be less parameters
                                uint8_t var_num = code[p++];
                                const uint16_t first_line = FROM_BCD_BE_16(code, p);
                                p+=2;
                                const uint16_t sec_line = FROM_BCD_BE_16(code, p);
                                p+=2;
                                out += " V"+int_to_hex(var_num) +", " + std::to_string(first_line) + ", " + std::to_string(sec_line);
                                break;
                            }
                        case 0x27: //DEFFN'
                            {
                                uint8_t oper_len = code[p++];
                                if (oper_len > 0) {
                                    const unsigned oper_start = p;
                                    const uint8_t gosub_id = code[p++];
                                    out += std::to_string(gosub_id);
                                    while (p < oper_start + oper_len) {
                                        out += "{"+int_to_hex(code[p++])+"}";
                                    }
                                }
                                break;
                            }
                        case 0x45: //BOOL
                            {
                                // operation code, two operands
                                uint8_t bool_code = code[p++];
                                out += ' ' + int_to_hex(bool_code) + '(';
                                break;
                            }
                        case 0x3F: //Short REM (%)
                        case 0x56: //REM
                            {
                                // <len> + string literal
                                uint8_t rem_len = code[p++];
                                for (uint8_t i=0; i<rem_len; i++) out += (*cm.charmap)[code[p++]];
                                break;
                            }
                        case 0x64: //INIT
                            {
                                // value, variable(s)
                                const uint8_t oper_len = code[p++];
                                const uint8_t first_oper = code[p];
                                if (first_oper == 0xDE) {
                                    p++;
                                    const uint8_t v = code[p++];
                                    out += "(" + int_to_hex(v) + ")";
                                    process_operands(verb_id, oper_len-2);
                                } else process_operands(verb_id, oper_len);
                                break;
                            }
                        default:
                            {
                                const uint8_t oper_len = code[p++];
                                if (oper_len > 0) process_operands(verb_id, oper_len);
                                break;
                            } //default
                    } //switch
                }

                out += "\n";

                if (p >= code.size()) break;
                if (code[p] != 0xFE) {
                    out += "Delimiter 0xFE not found, finishing\n";
                    break;
                }
                p+=1;
            }

        }


        return escapeHtml(out);
    }

}
