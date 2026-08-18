// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Iskra-226 BASIC


#include "viewer_basic_iskra226.h"

#include <charconv>
#include <cstring>
#include <stack>
#include <utility>

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
    "LET", "",                                                                       //35-36 LET & (LET)
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
    "DATA SAVE BA", "DATA SAVE DA", "DATA LOAD BA", "DATA LOAD DA", "?", "?",        //6E-73
    "DATA LOAD DC", "DATA LOAD DC OPEN T",                                           //74-75
    "?", "?", "DATA SAVE DC OPEN T",                                                 //76-78
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
    bool is_known;
    bool is_string;
    bool is_array;
    bool is_short;
};

enum class OperClass {Function, ImplFunc, Operation, Operand, VerbPart, Closer, VarRef, Error};

struct OperDefinition
{
    const char * as_operand;
    OperClass class_as_operand;
    const char * as_operation;
    OperClass class_as_operation;
    bool next_is_operand;
};

using table1_rec = uint16_t[4];

constexpr std::array<OperDefinition, 0x100-0xC0> Iskra226_operations = {{
    /* C0 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C1 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C2 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C3 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C4 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C5 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C6 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C7 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C8 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* C9 */ { "",     OperClass::Operation, "",       OperClass::Operation, true  },
    /* CA */ { "FROM", OperClass::VerbPart,  "FROM",   OperClass::VerbPart,  true  },
    /* CB */ { "ALL",  OperClass::VerbPart,  "ALL",    OperClass::VerbPart,  true  },   // RETURN CLEAR ...
    /* CC */ { "GOSUB",OperClass::VerbPart,  "GOSUB",  OperClass::VerbPart,  true  },   // ON ...
    /* CD */ { "GOTO", OperClass::VerbPart,  "GOTO",   OperClass::VerbPart,  true  },   // ON ...
    /* CE */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* CF */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* D0 */ { ")",      OperClass::Closer,    ")",    OperClass::Closer,    false },
    /* D1 */ { "TO",     OperClass::VerbPart,  "TO",   OperClass::VerbPart,  true  },
    /* D2 */ { "STEP",   OperClass::VerbPart,  "STEP", OperClass::VerbPart,  true  },
    /* D3 */ { "?THEN?", OperClass::Operand,   "THEN", OperClass::VerbPart,  true  },   // + 2 bytes BCD
    /* D4 */ { ">",      OperClass::Operation, ">",    OperClass::Operation, true  },
    /* D5 */ { "AT(",    OperClass::Operation, "<>",   OperClass::Operation, true  },
    /* D6 */ { "<=",     OperClass::Operation, "<=",   OperClass::Operation, true  },
    /* D7 */ { "<",      OperClass::Operation, "<",    OperClass::Operation, true  },
    /* D8 */ { ">=",     OperClass::Operation, ">=",   OperClass::Operation, true  },
    /* D9 */ { "=",      OperClass::Operation, "=",    OperClass::Operation, true  },
    /* DA */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* DB */ { "#",      OperClass::Operation, "#",    OperClass::Operation, true  },
    /* DC */ { "/",      OperClass::Operation, "/",    OperClass::Operation, true  },
    /* DD */ { ";",      OperClass::VerbPart,  ";",    OperClass::VerbPart,  true  },
    /* DE */ { ",",      OperClass::Operation, ",",    OperClass::Operation, true  },
    /* DF */ { "TAB(",   OperClass::Function,  "*",    OperClass::Operation, true  },
    /* E0 */ { "@",      OperClass::Operand,   "^",    OperClass::Operation, true  },   // операнд: ссылка на массив целиком, далее индекс
    /* E1 */ { "STR(",   OperClass::Function,  "STR(", OperClass::Function,  true  },
    /* E2 */ { "HEX(",   OperClass::Function,   "HEX(",OperClass::Function,  false },   // + байт длины + данные
    /* E3 */ { "\"",     OperClass::Operand,   "\"",   OperClass::Operand,   false },   // + байт длины + КОИ-8
    /* E4 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* E5 */ { "#",      OperClass::Operand,   "#",    OperClass::Operand,   false },   // + описатель + BCD
    /* E6 */ { "#",      OperClass::Operand,   "OR",   OperClass::Function,  false },   // константа 2 байта BCD + порядок
    /* E7 */ { "#",      OperClass::Operand,   "AND",  OperClass::Function,  false },   // константа 2 байта BCD
    /* E8 */ { "#",      OperClass::Operand,   "#",    OperClass::Operand,   false },   // + 1 байт BCD
    /* E9 */ { "-",      OperClass::Operation, "-",    OperClass::Operation, true  },   // унарный / бинарный
    /* EA */ { "+",      OperClass::Operation, "+",    OperClass::Operation, true  },
    /* EB */ { "(",      OperClass::Function,  "(",    OperClass::Function,  true  },
    /* EC */ { "POS(",   OperClass::ImplFunc,  "POS(", OperClass::ImplFunc,  true  },
    /* ED */ { "LEN(",   OperClass::ImplFunc,  "LEN(", OperClass::ImplFunc,  true  },
    /* EE */ { "NUM(",   OperClass::ImplFunc,  "NUM(", OperClass::ImplFunc,  true  },
    /* EF */ { "VAL(",   OperClass::ImplFunc,  "VAL(", OperClass::ImplFunc,  true  },
    /* F0 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* F1 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* F2 */ { "ABS(",   OperClass::Function,  "ABS(", OperClass::Function,  true  },
    /* F3 */ { "INT(",   OperClass::Function,  "INT(", OperClass::Function,  true  },
    /* F4 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* F5 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* F6 */ { "SQR(",   OperClass::Function,  "SQR(", OperClass::Function,  true  },
    /* F7 */ { "LOG(",   OperClass::Function,  "LOG(", OperClass::Function,  true  },
    /* F8 */ { "EXP(",   OperClass::Function,  "EXP(", OperClass::Function,  true  },
    /* F9 */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* FA */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* FB */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* FC */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* FD */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* FE */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },   // разделитель записей, вне выражений
    /* FF */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
}};

namespace dsk_tools {

    class Iskra226Parser
    {
    private:
            var_params all_vars[256];
            table1_rec * table1 = nullptr;
            uint8_t * table2 = nullptr;
            uint8_t * table3 = nullptr;
            unsigned L1;
            unsigned p;
            BYTES code={};
            CharmapInfo cm;
            bool is_operand = true;
            unsigned dim_vars = 0;
            bool is_let_left = false;
            bool impl_paren = false;
    public:
        Iskra226Parser(const CharmapInfo & charmap, table1_rec * table1, const unsigned L1):
            table1(table1),
            L1(L1),
            cm(charmap)
        {
            for (auto & var : all_vars) {var.is_known = false; var.is_string = false; var.is_array = false; var.is_short = false;}
        }

        static std::string verb_by_id(const uint16_t verb_id)
        {
            std::string verb;
            if (verb_id > 0xFF) {
                // Extended codes
                switch (verb_id)
                {
                case 0x0601: verb = "MAT"; break;
                case 0x0602: verb = "MAT REDIM"; break;
                case 0x060F: verb = "¤OPEN"; break;
                case 0x0615: verb = "DRAW"; break;
                case 0x0619: verb = "NPLOT"; break;
                case 0x061E: verb = "LABEL"; break;
                case 0x061F: verb = "¤COPY"; break;
                case 0x0625: verb = "ASMB"; break;
                default: verb = "?"; break;
                }
            } else if (verb_id < 0x84)
                verb = std::string(Iskra226_verbs[verb_id]);
            else
                verb = "?";
            if (verb == "?") verb = "?" + int_to_hex(verb_id) + "?";

            return verb;
        }

        static OperClass classify_oper(const uint8_t oper, const bool is_operand_place)
        {
            if (is_operand_place) {
                if (oper < 0xC0) return OperClass::VarRef;
                return Iskra226_operations[oper-0xC0].class_as_operand;
            }
            //if (oper < 0xC0) return OperClass::Error;
            if (oper < 0xC0) return OperClass::VarRef;
            return Iskra226_operations[oper-0xC0].class_as_operation;
        }

        std::string var_ref (const uint8_t b, const bool mark_arrays=true) const
        {
            std::string res = "V" + int_to_hex(b);
            if (all_vars[b].is_short) res += '%';
            if (all_vars[b].is_string) res += "¤";
            if (mark_arrays && all_vars[b].is_array) res += '(';
            return res;
        };

        static std::string device_letter (const uint8_t b)
        {
            switch (b) {
            case 0: return "F";
            case 1: return "R";
            default: return "???";
            }
        };

        // // Function returns true for POS(), LEN(), NUM(), VAL()
        // // which never have a closing parenthesis and should be processes special way
        // static bool implicit_paren (const uint8_t oper)
        // {
        //     return oper>=0xEC && oper <=0xEF;
        // };

        // Function return true for operations that should close an implicit parentesis
        static bool closes_implicit_paren (const uint8_t t)
        {
            switch (t) {
                // arithmetics
            case 0xE0:      // ^
            case 0xDF:      // *
            case 0xDC:      // /
            case 0xEA:      // +
            case 0xE9:      // -
                // conditions
            case 0xD4:      // >
            case 0xD5:      // <>
            case 0xD6:      // <=
            case 0xD7:      //
            case 0xD8:      // >=
            case 0xD9:      // =
                // logic
            case 0xE6:      // OR
            case 0xE7:      // AND
                // delimiters
            case 0xD0:      // )
            case 0xDD:      // ;
            // case 0xDE:      // ,
            case 0xDB:      // #
                // second keywords
            case 0xD1:      // TO
            case 0xD2:      // STEP
            case 0xD3:      // THEN
            case 0xCA:      // FROM
            case 0xCC:      // GOSUB at ON
            case 0xCD:      // GOTO at ON
                return true;
            default:
                return false;
            }
        };

        std::string process_operands(const uint16_t verb_id)
        {
            std::string out;
            is_operand = true;
            uint8_t prev_oper = 0xFF;
            while (p < code.size()) {
                const uint8_t oper_id = code[p++];
                const OperClass oper_class = classify_oper(oper_id, is_operand);
                std::string oper;
                if (oper_class != OperClass::VarRef && oper_class != OperClass::Error)
                    oper = is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation;

                // Var or literal on place of an operation can be:
                if (!is_operand && (oper_class == OperClass::VarRef || oper_class == OperClass::Operand || oper_class == OperClass::Function || oper_class == OperClass::ImplFunc))
                {
                    if (verb_id == 0x57) out += '='; // a left part of FOR
                    else
                    if (is_let_left) {
                        if (IS_VAR(prev_oper) && oper_class==OperClass::Operand && ! all_vars[prev_oper].is_known) {
                            all_vars[prev_oper].is_known = true;
                            all_vars[prev_oper].is_array = true;
                            out += '(';
                        }
                        if (!(IS_VAR(prev_oper) && all_vars[prev_oper].is_array)) out += ',';
                    } else {
                        if (!(IS_VAR(prev_oper) && all_vars[prev_oper].is_array) && verb_id != 0x0602) out += ',';
                    }
                }

                switch (oper_class)
                {
                case OperClass::Error:
                    {
                        out += "?ERROR?" + int_to_hex(oper_id) + '?';
                        break;
                    }
                case OperClass::VarRef:
                    {
                        out += var_ref(oper_id, true);
                        is_operand = false;
                        break;
                    }
                case OperClass::Operand:
                    {
                        switch (oper_id) {
                        case 0xE0: // Array reference
                            {
                                const uint8_t var_id = code[p++];
                                out += var_ref(var_id, false);
                                is_operand = false;
                                break;
                            }
                        case 0xE3: // String literal: <len> + KOI8 chars
                            {
                                const uint8_t str_len = code[p++];
                                out += '"';
                                for (int i=0; i<str_len; i++) {
                                    out += (*cm.charmap)[code[p++]];
                                }
                                out += '"';
                                is_operand = false;
                                break;
                            }
                        case 0xE7: // BCD 2 bytes big-endian constant
                            {
                                const uint16_t v = FROM_BCD_BE_16(code, p);
                                out += std::to_string(v);
                                p += 2;
                                is_operand = false;
                                break;
                            }
                        case 0xE8: // BCD byte constant
                            {
                                const uint8_t v = fromBCD(code[p++]);
                                out += std::to_string(v);
                                is_operand = false;
                            }
                        default:
                            break;
                        }
                        break;
                    }
                case OperClass::Operation:
                    {
                        if (is_operand) out += "?OPERATION?";

                        if (impl_paren && closes_implicit_paren(oper_id)) { out += ')'; impl_paren = false; }

                        if (oper_id == 0xD9 && is_let_left) is_let_left = false; // "=" cancels processing of the left part of LETs

                        out += std::string(is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation);
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        break;
                    }
                case OperClass::VerbPart:
                    {
                        if (!out.empty() && out.back()!=' ' && oper_id!=0xDD) out += ' ';
                        out += std::string(is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation);
                        switch (oper_id) {
                        case 0xCC: // ON ... GOSUB ...
                            {
                                unsigned c = 0;
                                while (p < code.size()) {
                                    const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                                    out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                                    p+=2;
                                }
                                break;
                            }
                        case 0xCD: // ON ... GOTO ...
                            {
                                unsigned c = 0;
                                while (p < code.size()) {
                                    const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                                    out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                                    p+=2;
                                }
                                break;
                            }
                        case 0xD3: //THEN + 2 bytes BCD
                            {
                                const uint16_t v = FROM_BCD_BE_16(code, p);
                                out += ' '+std::to_string(v);
                                p += 2;
                                break;
                            }
                        }
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        break;
                    }
                case OperClass::ImplFunc:
                case OperClass::Function:
                    {
                        if (oper_class == OperClass::ImplFunc) impl_paren = true;
                        switch (oper_id) {
                        case 0xE2: // HEX literal
                            {
                                const uint8_t hex_len = code[p++];
                                for (int i=0; i<hex_len; i++) oper += int_to_hex(code[p++]);
                                oper += ')';
                                is_operand = false;
                                break;
                            }
                        // case 0xE3: // String literal
                        //     {
                        //         const uint8_t str_len = code[p++];
                        //         out += '"';
                        //         for (int i=0; i<str_len; i++) {
                        //             out += (*cm.charmap)[code[p++]];
                        //         }
                        //         out += '"';
                        //         is_operand = false;
                        //         break;
                        //     }
                        default:
                            {
                                break;
                            }
                        }
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        out += oper;
                        break;
                    }
                default:
                    out += /*"?DEF?" + */std::string(is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation);
                    break;
                }
                prev_oper = oper_id;
            }

            return out;
        }

        std::string parse(const BYTES & line_code, const bool is_first)
        {
            std::string out;
            code = line_code;
            p = 0;
            impl_paren = false;

            out += "\n-> ";
            out += toHexList(code);
            out += '\n';
            if (!is_first) { out += ':'; }
            uint16_t verb_id = code[p++];
            if (verb_id == 0x06) verb_id = (verb_id << 8) + code[p++];

            // if (verb_id == 0x35 || verb_id == 0x36) preprocess_let();

            std::string verb = verb_by_id(verb_id);
            if (!verb.empty()) out +=  verb + ' ';

            const uint8_t oper_len = code[p++];
            if (oper_len > 0)
            {
                switch (verb_id) {
                case 0x21: //GOTO
                case 0x22: //GOSUB
                case 0x2F: //RUN
                    {
                        //len, 2 bytes BCD line number
                        const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                        out += std::to_string(goto_line);
                        p += 2;
                        break;
                    }
                case 0x25: //KEYIN
                    {
                        // var, line1, line2
                        uint8_t var_num = code[p++];
                        const uint16_t first_line = FROM_BCD_BE_16(code, p);
                        p+=2;
                        const uint16_t sec_line = FROM_BCD_BE_16(code, p);
                        p+=2;
                        out += var_ref(var_num) +", " + std::to_string(first_line) + ", " + std::to_string(sec_line);
                        break;
                    }
                case 0x27: //DEFFN'
                    {
                        if (oper_len > 0) {
                            const unsigned oper_start = p;
                            const uint8_t gosub_id = code[p++];
                            out += std::to_string(gosub_id);
                            p += 4; //Skip 4 shadow bytes
                            if (p < code.size()) process_operands(verb_id);
                        }
                        break;
                    }
                case 0x35: //LET
                case 0x36: //(LET)
                    {
                        is_let_left = true;
                        out += process_operands(verb_id);
                        break;
                    }
                case 0x3F: //Short REM (%)
                case 0x56: //REM
                    {
                        // string literal
                        for (uint8_t i=0; i<oper_len; i++) out += (*cm.charmap)[code[p++]];
                        break;
                    }
                case 0x40: //$GIO
                    {
                        uint8_t dev_id = code[p];
                        if (dev_id == 0xD5) {out += '\''; p++;};
                        out += process_operands(verb_id);
                        break;
                    }

                case 0x46: //DIM
                case 0x4E: //COM
                    {
                        // Variables list
                        bool first_var = true;
                        while (p < code.size()) {
                            const uint8_t var_id = code[p++];
                            if (!first_var) out += ','; first_var = false;
                            std::cout << int_to_hex(var_id) << std::endl;
                            if (table1 && dim_vars < L1)
                            {
                                //Table is reversed
                                const unsigned t1i = L1 - dim_vars - 1;
                                const auto ad = table1[t1i][0];
                                const auto ln = table1[t1i][2];
                                const auto sz = table1[t1i][3];
                                const uint32_t adelta = (dim_vars?(table1[t1i+1][0]):0x10000) - ad;
                                if (!all_vars[var_id].is_known) dim_vars++;
                                all_vars[var_id].is_known = true;
                                if (sz==4) {
                                    out += "V"+int_to_hex(var_id) + "%(" + std::to_string(ln) + ")";
                                    all_vars[var_id].is_short = true;
                                    all_vars[var_id].is_array = true;
                                } else if (sz==16) {
                                    out += "V"+int_to_hex(var_id) + "(" + std::to_string(ln) + ")";
                                    all_vars[var_id].is_array = true;
                                } else if (sz & 1) {
                                    all_vars[var_id].is_string = true;
                                    const unsigned str_len = (sz-1)/2;
                                    out += "V"+int_to_hex(var_id) + "¤";
                                    if (adelta > str_len + 6 ) {
                                        out += "(" + std::to_string(ln) + ")";
                                        all_vars[var_id].is_array = true;
                                    }
                                    out += std::to_string(str_len);
                                }
                                else out += " V"+int_to_hex(var_id) + "? ";
                            } else out += " V"+int_to_hex(var_id) + "? ";
                        }
                        break;
                    }
                case 0x4C: //PRINT
                    {
                        if (oper_len > 0) {
                            const uint8_t first_oper = code[p];
                            if (first_oper == 0xDC) {
                                p+=2; // skip DC DE
                                const uint8_t dev_id = code[p++];
                                out += '/'+int_to_hex(dev_id);
                                if (code[p]==0xDE) {p++; out += ',';}
                            } else if (first_oper == 0xD5) {
                                out += "AT(";
                                p++;
                            }
                            if (p < code.size()) out += process_operands(verb_id);
                            if (first_oper == 0xD5) out += ')';
                        }
                        break;
                    }
                case 0x54: //SELECT
                    {
                        const uint8_t dev_class = code[p];
                        if (dev_class == 0) {
                            //up to 4 numbers 00 01 18 00 = #118L, 00 04 1C 01 = #41CR
                            while (p<oper_len) {
                                uint8_t b0 = code[p++];
                                if (b0 == 0xDE) {
                                    out += ',';
                                    b0 = code[p++];
                                }
                                if (b0 != 0) {break;}
                                uint16_t n = FROM_BE_16(code, p); p+=2;
                                uint8_t b3 = code[p++];
                                out += "#" + int_to_hex(n, false) + device_letter(b3);
                            }
                        } else if (dev_class == 7) {
                            p++;
                            const uint8_t dev_id = code[p++];
                            out += "PRINT" + int_to_hex(dev_id);
                            if (code[p] == 0xEB) {
                                p++;
                                out += "(" + std::to_string(FROM_BCD_BE_16(code, p)) + ")"; p+=2;
                            }
                        } else {

                        }
                        break;
                    }

                case 0x64: //INIT
                    {
                        // value, variable(s)
                        const uint8_t first_oper = code[p];
                        if (first_oper == 0xDE) {
                            p++;
                            const uint8_t v = code[p++];
                            out += "(" + int_to_hex(v) + ")";
                        }
                        out += process_operands(verb_id);
                        break;
                    }

                default:
                    {
                        out += process_operands(verb_id);
                        break;
                    }
                }
            }
            if (impl_paren) out += ')';

            out += '\n';

            return out;
        }
    };

    std::string ViewerBASIC_Iskra226::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        std::string out;
        cm = init_charmap(cm_name);

        var_params all_vars[256];
        for (auto & var : all_vars) {var.is_known = false; var.is_string = false; var.is_array = false; var.is_short = false;}

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

            table1_rec * table1 = nullptr;
            uint8_t * table2 = nullptr;
            uint8_t * table3 = nullptr;

            if (L1 > 0) {
                table1 = reinterpret_cast<table1_rec*>(&code[6]);
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
            unsigned known_count = 0;
            bool is_operand = true;
            bool is_implicit_paren = false;

            auto process_float = [=](std::string & result, unsigned & lp)
            {
                result = "";
                const uint8_t num_def = code[lp++];
                const unsigned left_part = num_def >> 4;
                const unsigned dig_total = num_def & 0xF;
                const unsigned bcount = (dig_total & 1)? dig_total / 2 + 1 : dig_total / 2;
                for (int i=0; i<bcount; i++) result += toBCD(code[lp++]);

                // An odd digit count means the last nibble is a padding
                if (result.size() > dig_total) result.resize(dig_total);

                // left_part digits go before the dot, 0 means the dot leads the number
                if (left_part < result.size())
                    result.insert(left_part, ".");
                else
                    // Not enough digits for the integer part, pad it with zeroes
                        result.append(left_part - result.size(), '0');
            };

            auto device_letter = [](const uint8_t b) -> std::string
            {
                switch (b) {
                    case 0: return "F";
                    case 1: return "R";
                    default: return "???";
                }
            };

            auto var_ref = [&](const uint8_t b, const bool mark_arrays=true) -> std::string
            {
                std::string res = "V" + int_to_hex(b);
                if (all_vars[b].is_short) res += '%';
                if (all_vars[b].is_string) res += "¤";
                if (mark_arrays && all_vars[b].is_array) res += '(';
                // res += ' ';
                return res;
            };

            // Function returns true for POS(), LEN(), NUM(), VAL()
            // which never have a closing parenthesis and should be processes special way
            auto implicit_paren = [](const uint8_t oper) -> bool
            {
                return oper>=0xEC && oper <=0xEF;
            };

            // Function return true for operations that should close an implicit parentesis
            auto closes_implicit_paren = [](const uint8_t t) -> bool
            {
                switch (t) {
                    // arithmetics
                case 0xE0:      // ^
                case 0xDF:      // *
                case 0xDC:      // /
                case 0xEA:      // +
                case 0xE9:      // -
                    // conditions
                case 0xD4:      // >
                case 0xD5:      // <>
                case 0xD6:      // <=
                case 0xD7:      //
                case 0xD8:      // >=
                case 0xD9:      // =
                    // logic
                case 0xE6:      // OR
                case 0xE7:      // AND
                    // delimiters
                case 0xD0:      // )
                case 0xDD:      // ;
                case 0xDE:      // ,
                case 0xDB:      // #
                    // second keywords
                case 0xD1:      // TO
                case 0xD2:      // STEP
                case 0xD3:      // THEN
                case 0xCA:      // FROM
                case 0xCC:      // GOSUB at ON
                case 0xCD:      // GOTO at ON
                    return true;
                default:
                    return false;
                }
            };

            auto process_operands = [&](const uint16_t verb_id, const uint8_t oper_len)
            {
                const unsigned oper_start = p;
                unsigned oper_count = 0;
                unsigned prev = 1000; //Greater than any possible
                while (p < oper_start + oper_len) {
                    const uint8_t oper = code[p++];

                    if (implicit_paren(oper)) is_implicit_paren = true;
                    if (is_implicit_paren && closes_implicit_paren(oper)) {
                        is_implicit_paren = false;
                        out += ')';
                    }

                    if ((verb_id == 0x7D || verb_id == 0x80 || verb_id == 0x81) && oper_count==0) {
                        // First parameter of file instructions - a disk id
                        out += device_letter(oper);
                    } else if (IS_VAR(oper)) {
                        //Variable by index
                        if ((verb_id == 0x46 || verb_id == 0x4E) && table1)
                        {
                            // DIM, COM
                            if (oper_count>0) out += ',';
                            //Table is reversed
                            const unsigned t1i = L1/8 - known_count - 1;
                            const auto ad = table1[t1i][0];
                            const auto ln = table1[t1i][2];
                            const auto sz = table1[t1i][3];
                            const uint32_t adelta = (known_count?(table1[t1i+1][0]):0x10000) - ad;
                            if (!all_vars[oper].is_known) known_count++;
                            all_vars[oper].is_known = true;
                            if (sz==4) {
                                out += "V"+int_to_hex(oper) + "%(" + std::to_string(ln) + ")";
                                all_vars[oper].is_short = true;
                                all_vars[oper].is_array = true;
                            } else if (sz==16) {
                                out += "V"+int_to_hex(oper) + "(" + std::to_string(ln) + ")";
                                all_vars[oper].is_array = true;
                            } else if (sz & 1) {
                                all_vars[oper].is_string = true;
                                const unsigned str_len = (sz-1)/2;
                                out += "V"+int_to_hex(oper) + "¤";
                                if (adelta > str_len + 6 ) {
                                    out += "(" + std::to_string(ln) + ")";
                                    all_vars[oper].is_array = true;
                                }
                                out += std::to_string(str_len);
                            }
                            else out += " V"+int_to_hex(oper) + "? ";
                        } else if (verb_id == 0x0602) {
                            // MAT REDIM
                        } else {
                            // Insert a comma in a vars list, after a ")" and HEX()
                            if ((IS_VAR(prev) && !all_vars[prev].is_array) || prev == 0xD0 || prev == 0xE2) out += ',';
                            out += var_ref(oper);
                            if (prev == 0xE1 || prev == 0xE0) out += ','; //First argument of STR()
                            is_operand = false;
                            if (verb_id == 0x57 && oper_count == 0) out+='='; // "=" after first var in FOR
                        }
                    } else if (oper == 0xCC) {
                        out += " GOSUB"; // From ON ... GOSUB ...
                        unsigned c = 0;
                        while (p < oper_start + oper_len) {
                            const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                            out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                            p+=2;
                        }
                        // is_operand doesn't matter here
                    } else if (oper == 0xCD) {
                        out += " GOTO"; // From ON ... GOTO ...
                        unsigned c = 0;
                        while (p < oper_start + oper_len) {
                            const uint16_t goto_line = FROM_BCD_BE_16(code, p);
                            out += ((c++>0)?", ":" ") + std::to_string(goto_line);
                            p+=2;
                        }
                        // is_operand doesn't matter here
                    } else if (oper == 0xD3) {
                        out += " THEN";
                        const uint16_t v = FROM_BCD_BE_16(code, p);
                        out += ' '+std::to_string(v);
                        p += 2;
                        // is_operand doesn't matter here
                    } else if (oper == 0xDE && is_operand) {
                        // BCD byte constant
                        const uint8_t v = fromBCD(code[p++]);
                        out += std::to_string(v);
                        is_operand = false;
                    } else if (oper == 0xE0) {
                        // Array reference
                        if (verb_id != 0x0602) { // not MAT REDIM
                            const uint8_t var_n = code[p++];
                            out += "V" + int_to_hex(var_n) + "()";
                            is_operand = false;
                        } else {
                            const uint8_t var_n = code[p++];
                            out += var_ref(var_n, false);
                        }
                    } else if (oper == 0xE2) {
                        // HEX literal
                        const uint8_t hex_len = code[p++];
                        out += "HEX(";
                        for (int i=0; i<hex_len; i++) out += int_to_hex(code[p++]);
                        out += ')';
                        is_operand = false;
                    } else if (oper == 0xE3) {
                        // String literal
                        const uint8_t str_len = code[p++];
                        out += '"';
                        for (int i=0; i<str_len; i++) {
                            out += (*cm.charmap)[code[p++]];
                        }
                        out += '"';
                        is_operand = false;
                    } else if (oper == 0xE5) {
                        std::string num_str;
                        process_float(num_str, p);
                        out += num_str;
                        is_operand = false;
                    } else if (oper == 0xE6 && is_operand) {
                        std::string num_str;
                        process_float(num_str, p);
                        const uint8_t exp = code[p++];
                        out += num_str + 'E' + toBCD(exp);
                        is_operand = false;
                    } else if (oper == 0xE7 && is_operand) {
                        // BCD 2 bytes big-endian constant
                        const uint16_t v = FROM_BCD_BE_16(code, p);
                        out += std::to_string(v);
                        p += 2;
                        is_operand = false;
                    } else if (oper == 0xE8) {
                        // BCD byte constant
                        const uint8_t v = fromBCD(code[p++]);
                        out += std::to_string(v);
                        is_operand = false;
                    } else {
                        const OperDefinition * oper_def = &Iskra226_operations[oper-0xC0];
                        std::string oper_name;
                        if (is_operand)
                            oper_name = oper_def->as_operand;
                        else
                            oper_name= oper_def->as_operation;
                        if (oper_name.empty()) oper_name = "{" + int_to_hex(oper) + "}";
                        is_operand = oper_def->next_is_operand;

                        out += oper_name;
                    }
                    if (verb_id != 0x0602 || oper != 0xE0) oper_count++;
                    prev = oper;
                }
                if (is_implicit_paren) out += ')';
                is_implicit_paren = false;
            }; // process_operands

            // Check whether a token can begin an operand (that is, whether it
            // opens an index list if it follows a variable reference)
            // used below in preprocess_let()
            auto begins_operand = [](const uint8_t t) -> bool
            {
                if (t < 0xC0) return true;              // var reference
                switch (t) {
                    case 0xE5: case 0xE6:               // float constants
                    case 0xE7: case 0xE8:               // BCD-constants
                    case 0xEB:                          // "("
                    case 0xE9:                          // unary "-"
                    case 0xE1:                          // STR(
                    case 0xF2: case 0xF3:               // ABS( INT(
                    case 0xF6: case 0xF7: case 0xF8:    // SQR( LOG( EXP(
                        return true;
                    default:
                        return false;
                }
            }; // begins_operand

            // The function scans the left part of LETs
            // And tries to find array references
            auto preprocess_let = [&]() -> bool
            {
                const unsigned oper_start = p;
                unsigned lp = p;
                std::stack<uint8_t> vars_stack;
                std::stack<uint8_t> oper_stack;
                bool prev_ends_operand = false;
                std::string str;

                const uint8_t oper_len = code[lp++];
                while (lp < oper_start + oper_len)
                {
                    const uint8_t oper = code[lp++];
                    if (oper == 0xD9) break;            // "=" - left part ends

                    if (oper < 0xC0) {                  // variable reference
                        vars_stack.push(oper);
                        // if the next is an operand, open list
                        if (lp < oper_start + oper_len && begins_operand(code[lp]))
                            oper_stack.push(0xFE);
                        prev_ends_operand = true;
                    }
                    else if (oper == 0xE5) {            // float
                        vars_stack.push(0xFE);
                        process_float(str, lp);
                        prev_ends_operand = true;
                    }
                    else if (oper == 0xE6) {            // fload with order
                        vars_stack.push(0xFE);
                        process_float(str, lp);
                        lp++;
                        prev_ends_operand = true;
                    }
                    else if (oper == 0xE7) {            // BCD, 2 bytes
                        vars_stack.push(0xFE);
                        lp += 2;
                        prev_ends_operand = true;
                    }
                    else if (oper == 0xE8) {            // BCD, 1 byte
                        vars_stack.push(0xFE);
                        lp++;
                        prev_ends_operand = true;
                    }
                    else if (oper == 0xE9 && !prev_ends_operand) {
                        // unary minus, nothing changes, just skipping it
                    }
                    else if (oper == 0xD0) {            // ")"
                        // "execute" operations on the top of the stack
                        while (!oper_stack.empty() && oper_stack.top() != 0xFE
                               && vars_stack.size() > 1) {
                            oper_stack.pop();
                            vars_stack.pop(); vars_stack.pop();
                            vars_stack.push(0xFF);
                        }
                        if (oper_stack.empty() || oper_stack.top() != 0xFE) return false;
                        if (vars_stack.size() < 2) return false;

                        oper_stack.pop();
                        vars_stack.pop();
                        const uint8_t array_var = vars_stack.top(); vars_stack.pop();
                        vars_stack.push(0xFF);          // push "result"

                        if (!all_vars[array_var].is_known) {
                            all_vars[array_var].is_known = true;
                            all_vars[array_var].is_array  = true;
                        }
                        prev_ends_operand = true;
                    }
                    else {                              // прочие операции и разделители
                        oper_stack.push(oper);
                        prev_ends_operand = false;
                    }
                }

                return oper_stack.empty();
            }; // preprocess_let

            // auto classify_oper = [](const uint8_t oper, const bool is_operand) -> OperClass
            // {
            //
            // };
            //
            // auto parse_expression = [&](const unsigned oper_len) -> bool
            // {
            //     const unsigned oper_start = p;
            //     unsigned lp = p;
            //     std::stack<uint8_t> vars_stack;
            //     std::stack<uint8_t> oper_stack;
            //     while (lp < oper_start + oper_len) {
            //         uint8_t oper = code[lp];
            //         switch (classify_oper(oper, is_operand))
            //         {
            //
            //         }
            //
            //     }
            //     return true;
            // }; //parse_expression

            Iskra226Parser parser(cm, table1, L1/8);

            while (p < code.size()) {
                // In some cases a previous line can be padded by several zeroes to a full 256 segment
                // So we need to skip them
                if ((code.size()-p > 2) && code[p]==0 && code[p+1]==0) p+=2;

                const uint16_t line_num = FROM_BCD_BE_16(code, p);
                const uint8_t line_len = code[p+2];

                if (line_num > 0)
                    out += '\n' + toHexList(std::vector<uint8_t>(code.begin() + p, code.begin() + p + line_len + 3)) + '\n';

                p += 3;

                out+= pad_number(line_num, 4, '0') + " ";

                const unsigned line_start = p;
                const unsigned line_end = line_start + line_len - 1;

                while (p < line_end) {
                    const unsigned operator_start = p;
                    unsigned operator_len = 0;
                    uint16_t verb_id = code[p++];

                    if (verb_id == 0x35 || verb_id == 0x36) preprocess_let();

                    std::string verb;
                    if (verb_id == 0x06) {
                        // Extended codes
                        verb_id = (verb_id << 8) + code[p++];
                        switch (verb_id)
                        {
                            case 0x0601: verb = "MAT"; break;
                            case 0x0602: verb = "MAT REDIM"; break;
                            case 0x060F: verb = "¤OPEN"; break;
                            case 0x0615: verb = "DRAW"; break;
                            case 0x0619: verb = "NPLOT"; break;
                            case 0x061E: verb = "LABEL"; break;
                            case 0x061F: verb = "¤COPY"; break;
                            case 0x0625: verb = "ASMB"; break;
                            default: verb = "?"; break;
                        }
                    } else if (verb_id < 0x84)
                        verb = std::string(Iskra226_verbs[verb_id]);
                    else
                        verb = "?";
                    if (verb == "?") verb = "?" + int_to_hex(verb_id) + "?";

                    operator_len = code[p] + (verb_id<0x100 ? 2 : 3 );
                    const auto operator_code = BYTES(code.begin() + operator_start, code.begin() + operator_start + operator_len);

                    out += parser.parse(operator_code, p==line_start+1);

                    out += ((p > line_start + 1)?std::string(":"):"") +
                            // "[" + int_to_hex(static_cast<uint8_t>(verb_id)) + "]" +
                            verb + (verb.empty()?"":" "); // Do not add a space for (LET)

                    is_operand = true;


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
                                uint8_t oper_len = code[p++];
                                const uint16_t gosub_id = code[p++];
                                out += " " + std::to_string(gosub_id);
                                if (oper_len > 1){
                                    out += '(';
                                    process_operands(verb_id, oper_len-1);
                                    out += ')';
                                }
                                break;
                            }
                        // case 0x24: // IF
                        //     {
                        //         uint8_t oper_len = code[p++];
                        //         parse_expression(oper_len);
                        //     }
                        case 0x25: //KEYIN
                            {
                                // var, line1, line2
                                uint8_t oper_len = code[p++]; //TODO: check if it can be less parameters
                                uint8_t var_num = code[p++];
                                const uint16_t first_line = FROM_BCD_BE_16(code, p);
                                p+=2;
                                const uint16_t sec_line = FROM_BCD_BE_16(code, p);
                                p+=2;
                                out += var_ref(var_num) +", " + std::to_string(first_line) + ", " + std::to_string(sec_line);
                                break;
                            }
                        case 0x27: //DEFFN'
                            {
                                uint8_t oper_len = code[p++];
                                if (oper_len > 0) {
                                    const unsigned oper_start = p;
                                    const uint8_t gosub_id = code[p++];
                                    out += std::to_string(gosub_id);
                                    // while (p < oper_start + oper_len) {
                                    //     out += "{"+int_to_hex(code[p++])+"}";
                                    // }
                                    p += 4; //Skip 4 shadow bytes
                                    if (p < line_end) process_operands(verb_id, oper_len-5);
                                }
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
                        case 0x40: //$GIO
                            {
                                uint8_t oper_len = code[p++];
                                uint8_t dev_id = code[p];
                                if (dev_id == 0xD5) {out += '\''; p++; oper_len--;};
                                if (p < line_end) process_operands(verb_id, oper_len);
                                break;
                            }
                        case 0x45: //BOOL
                            {
                                // operation code, two operands
                                uint8_t bool_code = code[p++];
                                out += ' ' + int_to_hex(bool_code) + '(';
                                break;
                            }
                        case 0x4C: //PRINT
                            {
                                uint8_t oper_len = code[p++];
                                if (oper_len > 0) {
                                    const uint8_t first_oper = code[p];
                                    if (first_oper == 0xDC) {
                                        p+=2; oper_len-=2; // skip DC DE
                                        const uint8_t dev_id = code[p++]; oper_len--;
                                        out += '/'+int_to_hex(dev_id);
                                        if (code[p]==0xDE) {p++; oper_len--; out += ',';}
                                    }
                                    if (p < line_end) process_operands(verb_id, oper_len);
                                }
                                break;
                            }
                        case 0x54: //SELECT
                            {
                                const uint8_t oper_len = code[p++];
                                const uint8_t dev_class = code[p];
                                if (dev_class == 0)
                                {
                                    //up to 4 numbers 00 01 18 00 = #118L, 00 04 1C 01 = #41CR
                                    for (int i=0; i<4; i++) {
                                        if (line_end - p < 4) {p = line_end; break;}
                                        uint8_t b0 = code[p++];
                                        if (b0 == 0xDE) {
                                            out += ',';
                                            b0 = code[p++];
                                        }
                                        if (b0 != 0) {p = line_end; break;}
                                        uint16_t n = FROM_BE_16(code, p); p+=2;
                                        uint8_t b3 = code[p++];
                                        out += "#" + int_to_hex(n, false) + device_letter(b3);
                                    }
                                } else if (dev_class == 7) {
                                    p++;
                                    const uint8_t dev_id = code[p++];
                                    out += "PRINT" + int_to_hex(dev_id);
                                    if (code[p] == 0xEB) {
                                        p++;
                                        out += "(" + std::to_string(FROM_BCD_BE_16(code, p)) + ")"; p+=2;
                                    }
                                } else {

                                }
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
                        case 0x66: //DATA LOAD BT
                        case 0x68: //DATA SAVE BT
                        case 0x6E: //DATA SAVE BA
                        case 0x6F: //DATA SAVE DA
                        case 0x70: //DATA LOAD BA
                        case 0x71: //DATA LOAD DA
                        case 0x74: //DATA LOAD DC
                        case 0x75: //DATA LOAD DC OPEN T
                        case 0x78: //DATA SAVE DC OPEN T
                            {
                                uint8_t oper_len = code[p++];
                                const uint8_t first_oper = code[p];
                                if (first_oper == 0x02) { // "T" form
                                    out += 'T'; p++; oper_len--;
                                    const uint8_t second_oper = code[p];
                                    if (second_oper == 0xD6) {out+="¤"; p++; oper_len--;}
                                } else if (first_oper == 0xDB) {  // file ops
                                    // nothing special?
                                } else if (first_oper == 0xDC) {  // device ops
                                    p+=2; oper_len-=2; //skip DC DB
                                    uint8_t device_id = code[p++]; oper_len--;
                                    out += '/'+std::to_string(device_id);
                                }
                                if (oper_len > 0) process_operands(verb_id, oper_len);
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
