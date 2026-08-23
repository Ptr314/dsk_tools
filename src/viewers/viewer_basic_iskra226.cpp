// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Iskra-226 BASIC


#include "viewer_basic_iskra226.h"

#include <cstring>
#include <stack>
#include <utility>

#include "utils.h"

#define FROM_BE_16(a, n) (a[n] << 8 | a[n+1])
#define FROM_BCD_BE_16(a, n) ((fromBCD(a[n]) * 100) + fromBCD(a[n+1]))

constexpr std::array<const char*, 0x84> Iskra226_verbs = {
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",  //00-0F
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",            //10-1D
    "IF END THEN",                                                                   //1E
    "?",                                                                             //1F
    "?",                                                                             //20
    "GOTO", "GOSUB", "GOSUB'", "IF", "KEYIN", "ON", "DEFFN'", "PRINTUSING", "DATA",  //21-29
    "SAVE", "RENUMBER", "CLEAR", "LOAD", "LIST", "RUN",                              //2A-2F
    "RETURN CLEAR", "?", "?", "?", "ON ERROR",                                       //30-34
    "LET", "",                                                                       //35-36 LET & (LET)
    "?", "?", "?",                                                                   //37-39
    "DEFFN'",                                                                        //3A a text (key) definition
    "?", "?", "?", "?",                                                              //3B-3E
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
    "DATA LOAD DC", "DATA LOAD DC OPEN",                                             //74-75
    "DATA SAVE DC", "DATA SAVE DC CLOSE", "DATA SAVE DC OPEN",                       //76-78
    "DBACKSPACE", "DSKIP", "LIMITS",                                                 //79-7B
    "LIST DC", "LOAD DC",                                                            //7C-7D
    "MOVE",                                                                          //7E
    "?", "SAVE DC",                                                                  //7F-80
    "SCRATCH",                                                                       //81
    "SCRATCH DISK",                                                                  //82
    "VERIFY"                                                                         //83
};

// Variable references occupy 00..C9, the operand token table starts at CA
constexpr uint8_t VAR_MAX = 0xC9;

// Payload of one program sector: 256 bytes minus the two service ones
constexpr unsigned SECTOR_DATA = 254;

#define IS_VAR(n)   (n <= VAR_MAX)

struct var_params
{
    bool is_known;          // described by a record of tables 2/3
    bool is_string;         // "¤" suffix
    bool is_short;          // "%" suffix (integer)
    bool is_array;
    bool shape_unknown;     // has a table 1 descriptor, but array/scalar is undecidable
    bool has_len;           // string length was given explicitly
    unsigned str_len;
    unsigned dim1;
    unsigned dim2;          // 0 for one-dimensional arrays
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
    /* D2 */ { "T",      OperClass::VerbPart,  "STEP", OperClass::VerbPart,  true  },   // as operand: SAVE DC ...T(...)
    /* D3 */ { "?THEN?", OperClass::Operand,   "THEN", OperClass::VerbPart,  true  },   // + 2 bytes BCD
    /* D4 */ { ">",      OperClass::Operation, ">",    OperClass::Operation, true  },
    /* D5 */ { "AT(",    OperClass::ImplFunc,  "<>",   OperClass::Operation, true  },   // AT( has no closing token
    /* D6 */ { "BEG",    OperClass::VerbPart,  "<=",   OperClass::Operation, true  },   // as operand: BEG / "¤" of DATA SAVE DC
    /* D7 */ { "END",    OperClass::VerbPart,  "<",    OperClass::Operation, true  },
    /* D8 */ { "ROUND(", OperClass::Function,  ">=",   OperClass::Operation, true  },
    /* D9 */ { "=",      OperClass::Operation, "=",    OperClass::Operation, true  },
    /* DA */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
    /* DB */ { "#",      OperClass::Operand,   "#",    OperClass::Operation, true  },   // as operand: a file number
    /* DC */ { "/",      OperClass::Operand,   "/",    OperClass::Operation, true  },   // as operand: device address prefix
    /* DD */ { ";",      OperClass::VerbPart,  ";",    OperClass::VerbPart,  true  },
    /* DE */ { "#",      OperClass::Operand,   ",",    OperClass::Operation, true  },   // as operand: 1 byte constant
    /* DF */ { "TAB(",   OperClass::Function,  "*",    OperClass::Operation, true  },
    /* E0 */ { "@",      OperClass::Operand,   "^",    OperClass::Operation, true  },   // операнд: ссылка на массив целиком, далее индекс
    /* E1 */ { "STR(",   OperClass::Function,  "STR(", OperClass::Function,  true  },
    /* E2 */ { "HEX(",   OperClass::Operand,   "HEX(",OperClass::Operand,    false },   // + байт длины + данные
    /* E3 */ { "\"",     OperClass::Operand,   "\"",   OperClass::Operand,   false },   // + байт длины + КОИ-8
    /* E4 */ { "'",      OperClass::Operand,   "'",    OperClass::Operand,   false },   // literal in apostrophes, see E3
    /* E5 */ { "#",      OperClass::Operand,   "#",    OperClass::Operand,   false },   // + описатель + BCD
    /* E6 */ { "#",      OperClass::Operand,   "OR",   OperClass::Operation, true  },   // + описатель + BCD + порядок
    /* E7 */ { "#",      OperClass::Operand,   "AND",  OperClass::Operation, true  },   // константа 2 байта BCD
    /* E8 */ { "#",      OperClass::Operand,   "#",    OperClass::Operand,   false },   // + 1 байт BCD
    /* E9 */ { "-",      OperClass::Operation, "-",    OperClass::Operation, true  },   // унарный / бинарный
    /* EA */ { "+",      OperClass::Operation, "+",    OperClass::Operation, true  },
    /* EB */ { "(",      OperClass::Function,  "(",    OperClass::Function,  true  },
    /* EC */ { "POS(",   OperClass::ImplFunc,  "POS(", OperClass::ImplFunc,  true  },
    /* ED */ { "LEN(",   OperClass::ImplFunc,  "LEN(", OperClass::ImplFunc,  true  },
    /* EE */ { "NUM(",   OperClass::ImplFunc,  "NUM(", OperClass::ImplFunc,  true  },
    /* EF */ { "VAL(",   OperClass::ImplFunc,  "VAL(", OperClass::ImplFunc,  true  },
    /* F0 */ { "?F0?(",  OperClass::Function,  "?F0?(",OperClass::Function,  true  },   // one of SIN/COS/TAN/ARCSIN/ARCCOS
    /* F1 */ { "#PI",    OperClass::Operand,   "#PI",  OperClass::Operand,   false },
    /* F2 */ { "ABS(",   OperClass::Function,  "ABS(", OperClass::Function,  true  },
    /* F3 */ { "INT(",   OperClass::Function,  "INT(", OperClass::Function,  true  },
    /* F4 */ { "RND(",   OperClass::Function,  "RND(", OperClass::Function,  true  },
    /* F5 */ { "SGN(",   OperClass::Function,  "SGN(", OperClass::Function,  true  },
    /* F6 */ { "SQR(",   OperClass::Function,  "SQR(", OperClass::Function,  true  },
    /* F7 */ { "LOG(",   OperClass::Function,  "LOG(", OperClass::Function,  true  },
    /* F8 */ { "EXP(",   OperClass::Function,  "EXP(", OperClass::Function,  true  },
    /* F9 */ { "?F9?(",  OperClass::Function,  "?F9?(",OperClass::Function,  true  },   // one of SIN/COS/TAN/ARCSIN/ARCCOS
    /* FA */ { "?FA?(",  OperClass::Function,  "?FA?(",OperClass::Function,  true  },
    /* FB */ { "?FB?(",  OperClass::Function,  "?FB?(",OperClass::Function,  true  },
    /* FC */ { "?FC?(",  OperClass::Function,  "?FC?(",OperClass::Function,  true  },
    /* FD */ { "ARCTAN(",OperClass::Function,  "ARCTAN(",OperClass::Function,true  },
    /* FE */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },   // разделитель записей, вне выражений
    /* FF */ { "",       OperClass::Operation, "",     OperClass::Operation, true  },
}};

namespace dsk_tools {

    class Iskra226Parser
    {
    private:
            var_params all_vars[256] = {};
            unsigned p = 0;
            BYTES code={};
            CharmapInfo cm;
            bool is_operand = true;
            bool is_let_left = false;
            // One entry per open explicit parenthesis, holding the number of implicit
            // functions opened inside it. Implicit functions (POS, LEN, NUM, VAL, AT)
            // have no closing token: they end at the first operation of their own level,
            // and a D0 belonging to an inner explicit parenthesis must not close them.
            struct paren_level
            {
                unsigned impl;   // POS(, LEN(, NUM(, VAL( - end at the first operation
                unsigned at;     // AT( - takes a whole list and ends with the statement
                bool     group;  // opened by EB, not by an index list or a function
            };
            std::vector<paren_level> impl_stack;
            bool is_pos_func = false;
            bool bin_target = false;   // BIN( is still inside its target operand
            bool wrap_open = false;     // an implicit parenthesis is open until the statement ends
            bool closed_group = false;  // the last D0 closed an explicit (...) group
            bool is_val_func = false;
    public:
        explicit Iskra226Parser(const CharmapInfo & charmap):
            cm(charmap)
        {}

        // Builds the variable model out of tables 2/3 and table 1.
        //
        // Tables 2 and 3 form a single array of 4-byte descriptors, one per variable,
        // ordered by DESCENDING variable index: N = L2/4 + L3/4, and the record for
        // index i sits at position N-1-i (table 2 first, then table 3).
        // Flag bit 5 marks a string, bit 4 a real number, neither an integer ("%").
        // Flag bit 0 marks a variable that also has a table 1 descriptor; the k-th
        // such record corresponds to the k-th record of table 1.
        void build_vars(const BYTES & stream, const unsigned L1, const unsigned L2, const unsigned L3)
        {
            const unsigned n1 = L1 / 8;
            const unsigned n2 = L2 / 4;
            const unsigned n3 = L3 / 4;
            const unsigned N  = n2 + n3;
            unsigned k = 0;

            for (unsigned pos = 0; pos < N; pos++) {
                const unsigned base = (pos < n2) ? (6 + L1 + pos*4)
                                                 : (6 + L1 + L2 + (pos - n2)*4);
                if (base + 4 > stream.size()) break;
                const unsigned idx = N - 1 - pos;
                if (idx > VAR_MAX) continue;

                const uint8_t flag = stream[base + 2];
                var_params & v = all_vars[idx];
                v.is_known  = true;
                v.is_string = (flag & 0x20) != 0;
                v.is_short  = (flag & 0x30) == 0;

                if ((flag & 1) == 0 || k >= n1) continue;

                const unsigned t = 6 + k*8;
                k++;
                if (t + 8 > stream.size()) continue;
                const uint32_t t_addr = stream[t]   | (stream[t+1] << 8);
                const uint16_t t_type = stream[t+2] | (stream[t+3] << 8);
                const uint16_t t_n    = stream[t+4] | (stream[t+5] << 8);
                const uint16_t t_sz   = stream[t+6] | (stream[t+7] << 8);

                // Bytes 2-3 hold the second dimension; 08xx cannot be one, so it means
                // "one-dimensional". Implicitly declared arrays always have ten elements
                // and carry 0127 or 0759 there — not dimensions either.
                bool one_dim = ((t_type >> 8) == 0x08)
                            || (t_n == 10 && (t_type == 0x0127 || t_type == 0x0759));

                v.has_len = (t_sz & 1) != 0;
                v.dim1 = t_n;
                v.dim2 = one_dim ? 0 : t_type;

                if (!v.is_string) {
                    v.is_array = true;
                    continue;
                }

                v.str_len = v.has_len ? (t_sz - 1) / 2 : t_sz / 2;

                // A scalar string of default length gets no table 1 descriptor at all,
                // so an even size code always means an array. The same for two dimensions.
                if (!v.has_len || !one_dim) { v.is_array = true; continue; }

                // Otherwise only the distance to the next descriptor tells them apart.
                // Addresses are zero in files that were never executed, and scalars
                // without a descriptor may sit in between, so the distance is an upper
                // bound rather than the exact size.
                const bool     has_next = (k < n1) && (t + 16 <= stream.size());
                const uint32_t next_ad  = has_next ? (stream[t+8] | (stream[t+9] << 8)) : 0xFFFE;
                if (t_addr != 0 && next_ad > t_addr) {
                    const uint32_t adelta    = next_ad - t_addr;
                    const uint32_t size_arr  = static_cast<uint32_t>(t_n) * v.str_len + 6;
                    const uint32_t size_sc   = v.str_len + (v.str_len & 1);
                    if (size_arr == adelta && size_sc != adelta) { v.is_array = true; continue; }
                    if (size_sc == adelta && size_arr != adelta) continue;   // a scalar
                    if (size_arr > adelta) continue;                         // cannot be an array
                }
                v.shape_unknown = true;
            }
        }

        static std::string verb_by_id(const uint16_t verb_id)
        {
            std::string verb;
            if (verb_id > 0xFF) {
                // Extended codes
                switch (verb_id)
                {
                case 0x0600: verb = "PLOT"; break;           // probable
                case 0x0601: verb = "MAT"; break;
                case 0x0602: verb = "MAT REDIM"; break;
                case 0x0603: verb = "MAT READ"; break;
                case 0x0604: verb = "MAT INPUT"; break;
                case 0x0606: verb = "MAT COPY"; break;
                case 0x060A: verb = "MAT SEARCH"; break;
                case 0x060C: verb = "¤TRAN("; break;
                case 0x060F: verb = "¤OPEN"; break;
                case 0x0613: verb = "DOT"; break;
                case 0x0614: verb = "DDRAW"; break;
                case 0x0615: verb = "DRAW"; break;
                case 0x0619: verb = "NPLOT"; break;
                case 0x061E: verb = "LABEL"; break;
                case 0x061C: verb = "STRETCH"; break;
                case 0x061F: verb = "¤COPY"; break;
                case 0x0622: verb = "¤LET"; break;
                case 0x0623: verb = "WINDOW"; break;
                case 0x0624: verb = "LINPUT"; break;
                case 0x0625: verb = "ASMB"; break;
                case 0x0626: verb = "REPLACE"; break;
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
                if (IS_VAR(oper)) return OperClass::VarRef;
                return Iskra226_operations[oper-0xC0].class_as_operand;
            }
            if (IS_VAR(oper)) return OperClass::VarRef;
            return Iskra226_operations[oper-0xC0].class_as_operation;
        }

        std::string var_ref(const uint8_t b, const bool mark_arrays=true) const
        {
            std::string res = "V" + int_to_hex(b);
            if (all_vars[b].is_short) res += '%';
            if (all_vars[b].is_string) res += "¤";
            if (mark_arrays && all_vars[b].is_array) res += '(';
            return res;
        };

        // Full declaration as it would appear in DIM/COM
        std::string var_decl(const uint8_t b) const
        {
            const var_params & v = all_vars[b];
            std::string res = "V" + int_to_hex(b);
            if (v.is_short) res += '%';
            if (v.is_string) res += "¤";
            if (v.is_array) {
                res += "(" + std::to_string(v.dim1);
                if (v.dim2 != 0) res += "," + std::to_string(v.dim2);
                res += ")";
            } else if (v.shape_unknown) {
                res += "?";
            }
            if (v.is_string && v.has_len) res += std::to_string(v.str_len);
            return res;
        };

        // First operand byte of disk statements: device or access mode
        static std::string device_letter(const uint8_t b)
        {
            switch (b) {
            case 0: return "F";
            case 1: return "R";
            case 2: return "T";
            default: return "???";
            }
        };

        // Function return true for operations that should close an implicit parentesis
        static bool closes_implicit_paren(const uint8_t t)
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
            case 0xD7:      // <
            case 0xD8:      // >=
            case 0xD9:      // =
                // logic
            case 0xE6:      // OR
            case 0xE7:      // AND
                // delimiters
            case 0xD0:      // )
            case 0xDD:      // ;
            case 0xDE:      // , - except the ",2" of VAL(, handled separately
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

        static bool conditions(const uint8_t t)
        {
            switch (t) {
            case 0xD4:      // >
            case 0xD5:      // <>
            case 0xD6:      // <=
            case 0xD7:      // <
            case 0xD8:      // >=
            case 0xD9:      // =
                return true;
            default:
                return false;
            }
        };

        // E5/E6 numeric constant: a descriptor byte whose high nibble is the number of
        // digits before the point and whose low nibble is the total number of digits,
        // then the digits themselves packed two per byte. E6 adds an exponent byte.
        // Integers wider than four digits also use this form (E7 holds up to 9999).
        std::string decode_number(const bool with_exponent)
        {
            if (p >= code.size()) return "?";
            const uint8_t d = code[p++];
            const unsigned before = d >> 4;
            const unsigned total  = d & 0x0F;

            std::string digits;
            for (unsigned i = 0; i < (total + 1) / 2 && p < code.size(); i++) {
                const uint8_t b = code[p++];
                digits += static_cast<char>('0' + (b >> 4));
                digits += static_cast<char>('0' + (b & 0x0F));
            }
            if (digits.size() > total) digits.resize(total);

            std::string res;
            if (total == 0)            res = "0";
            else if (before == 0)      res = "." + digits;
            else if (before >= total)  res = digits + std::string(before - total, '0');
            else                       res = digits.substr(0, before) + "." + digits.substr(before);

            if (with_exponent && p < code.size()) res += "E" + std::to_string(fromBCD(code[p++]));
            return res;
        };

        // Tokens that can only be read as an operation, whatever the parser expects.
        // Used to recover after a whole-array reference, which may be followed either
        // by an index list (STR(A(),1)) or by an operation (MAT A()=B()).
        static bool operation_only(const uint8_t t)
        {
            switch (t) {
            case 0xD0: case 0xD1: case 0xD2: case 0xD3:
            case 0xD4: case 0xD5: case 0xD6: case 0xD7: case 0xD8: case 0xD9:
            case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            case 0xE0: case 0xEA:
                return true;
            default:
                return false;
            }
        };

        // Tokens that can only start an operand. A variable followed by one of them
        // has opened an index list, because lists of receiving variables never
        // contain constants or function calls.
        static bool opens_index(const uint8_t t)
        {
            if (t == 0xD5 || t == 0xD8 || t == 0xDF) return true;         // AT(, ROUND(, TAB(
            if (t >= 0xE1 && t <= 0xE3) return true;                      // STR(, HEX(, literal
            if (t >= 0xE5 && t <= 0xE8) return true;                      // numeric constants
            if (t == 0xEB) return true;                                   // (
            if (t >= 0xEC && t <= 0xFD) return true;                      // functions and #PI
            return false;
        };

        // Statements whose operands are a plain list of values: no exponentiation can
        // occur there, so E0 is always a reference to a whole array
        static bool is_value_list(const uint16_t verb_id)
        {
            switch (verb_id) {
            case 0x40:                                               // ¤GIO
            case 0x64:                                               // INIT
            case 0x41: case 0x44:                                    // INPUT, READ
            case 0x66: case 0x68: case 0x6E: case 0x6F:              // DATA LOAD/SAVE BT, BA, DA
            case 0x70: case 0x71: case 0x74: case 0x75:
            case 0x76: case 0x78:                                    // DATA LOAD/SAVE DC
            case 0x7B:                                               // LIMITS
            case 0x0603: case 0x0604: case 0x0624:                   // MAT READ, MAT INPUT, LINPUT
                return true;
            default:
                return false;
            }
        };

        static bool add_spaces(const uint8_t t)
        {
            switch (t) {
            case 0xE6:      // OR
            case 0xE7:      // AND
                return true;
            default:
                return false;
            }
        };

        // Closes every implicit function opened at the innermost explicit level.
        // AT( is left alone: it swallows a whole argument list.
        void close_implicit(std::string & out)
        {
            while (!impl_stack.empty() && impl_stack.back().impl > 0) {
                out += ')';
                impl_stack.back().impl--;
            }
        };

        // Closes everything still open, innermost level first
        void close_all_implicit(std::string & out)
        {
            while (!impl_stack.empty()) {
                close_implicit(out);
                while (impl_stack.back().at > 0) { out += ')'; impl_stack.back().at--; }
                if (impl_stack.size() == 1) break;
                impl_stack.pop_back();
            }
        };

        std::string process_operands(const uint16_t verb_id, const bool start_as_operand = true)
        {
            std::string out;
            is_operand = start_as_operand;
            uint8_t prev_oper = 0xFF;
            unsigned token_no = 0;
            while (p < code.size()) {
                const uint8_t oper_id = code[p++];
                token_no++;

                // FOR always assigns to its loop variable, the "=" is not encoded
                if (verb_id == 0x57 && token_no == 2) {
                    out += '=';
                    is_operand = true;
                }

                // The second argument of VAL( is written as DE DB
                if (is_val_func && prev_oper == 0xDE && oper_id == 0xDB) {
                    is_val_func = false;
                    out += '2';
                    is_operand = false;
                    prev_oper = oper_id;
                    continue;
                }

                // DATA is a bare list of values without any separators, and MAT REDIM
                // gives the element length right after the closing parenthesis, so
                // E5/E6/E7 in both are always constants and never OR/AND
                if ((verb_id == 0x29 || verb_id == 0x0602) && !is_operand
                    && (oper_id == 0xE5 || oper_id == 0xE6 || oper_id == 0xE7)) {
                    if (verb_id == 0x29) out += ',';   // MAT REDIM writes A¤(n)len
                    is_operand = true;
                }

                // An omitted parameter is encoded by nothing at all, so a DE right after
                // another DE (or at the very beginning) is a comma, not a one byte constant
                if (oper_id == 0xDE && (prev_oper == 0xDE || (token_no == 1 && verb_id != 0x64)))
                    is_operand = false;

                // AND(, OR(, XOR( accept a bare hexadecimal byte as a mask
                if (is_operand && p >= code.size() && oper_id > VAR_MAX
                    && (verb_id == 0x43 || verb_id == 0x61 || verb_id == 0x62 || verb_id == 0x63)) {
                    out += int_to_hex(oper_id);
                    is_operand = false;
                    prev_oper = oper_id;
                    continue;
                }

                // ROTATE C(...) - the optional carry modifier
                if (oper_id == 0xD4 && verb_id == 0x4D && token_no == 1) {
                    out += 'C';
                    prev_oper = oper_id;
                    continue;
                }

                // PLOT writes its argument groups in angle brackets, and its third
                // element (the pen) is a keyword token rather than a constant
                if (verb_id == 0x0600) {
                    if (oper_id == 0xD7 && is_operand) {
                        out += '<';
                        prev_oper = oper_id;
                        continue;
                    }
                    if (oper_id == 0xD4) {
                        out += '>';
                        is_operand = false;
                        prev_oper = oper_id;
                        continue;
                    }
                    if (is_operand && (oper_id == 0xE5 || oper_id == 0xE6)) {
                        out += "?" + int_to_hex(oper_id) + "?";
                        is_operand = false;
                        prev_oper = oper_id;
                        continue;
                    }
                }

                // VERIFY T#n,(<sector>,<sector>) - the parentheses are not encoded
                if (verb_id == 0x83 && !wrap_open && oper_id == 0xDE && impl_stack.size() == 1) {
                    out += ",(";
                    wrap_open = true;
                    is_operand = true;
                    prev_oper = oper_id;
                    continue;
                }

                // ¤TRAN(A¤,B¤)R - the comma after the closing parenthesis is not printed
                if (verb_id == 0x060C && oper_id == 0xDE && prev_oper == 0xD0) {
                    is_operand = true;
                    prev_oper = oper_id;
                    continue;
                }

                // BIN( <target> [,2] ) = <value> : neither the closing parenthesis nor
                // the "=" is encoded
                if (bin_target && oper_id == 0xDB && prev_oper == 0xDE) {
                    out += "2)=";
                    bin_target = false;
                    is_operand = true;
                    prev_oper = oper_id;
                    continue;
                }

                // MAT SEARCH puts the comparison sign where an operand is expected
                if (verb_id == 0x060A && is_operand && oper_id >= 0xD4 && oper_id <= 0xD9)
                    is_operand = false;

                // In INPUT and LINPUT a "-" can only start the next item of the list
                if (oper_id == 0xE9 && !is_operand && impl_stack.size() == 1
                    && (verb_id == 0x41 || verb_id == 0x0624)) {
                    out += ',';
                    is_operand = true;
                }

                OperClass oper_class = classify_oper(oper_id, is_operand);

                // In a list of values E0 is a whole array and never the "^" operation.
                // is_operand stays false so that the list separator is still printed.
                if (oper_id == 0xE0 && !is_operand && is_value_list(verb_id))
                    oper_class = OperClass::Operand;
                // A reference to an array leaves the parser expecting an index list, but
                // MAT A()=B() and NPLOT A(),x continue with an operation instead. The same
                // happens when a string variable was wrongly taken for an array.
                if (operation_only(oper_id) && is_operand
                    && (prev_oper == 0xE0 || (IS_VAR(prev_oper) && all_vars[prev_oper].is_array))) {
                    oper_class = classify_oper(oper_id, false);
                    is_operand = false;
                    // the index list was never opened after all
                    if (IS_VAR(prev_oper) && impl_stack.size() > 1) impl_stack.pop_back();
                }

                std::string oper;
                if (oper_class != OperClass::VarRef && oper_class != OperClass::Error)
                    oper = is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation;

                if (oper_class == OperClass::VarRef || oper_class == OperClass::Operand || oper_class == OperClass::Function || oper_class == OperClass::ImplFunc) {
                    if (is_operand) {
                        if (IS_VAR(prev_oper) && all_vars[prev_oper].is_array) {
                           out+='(';
                        }
                    } else {
                        // An operand where an operation was expected: an implicit list
                        // separator. Lists of receiving variables (INPUT, DIM, DATA LOAD,
                        // LIMITS, the left part of LET) carry no separator token at all,
                        // and neither does the first comma of STR( or the one after the
                        // prompt of INPUT.
                        if (bin_target && impl_stack.size() == 1) {
                            // the target of BIN( has ended, the value follows
                            out += ")=";
                            bin_target = false;
                        }
                        // CONVERT writes its image in parentheses: TO A¤,(###)
                        else if (verb_id == 0x47 && !wrap_open && impl_stack.size() == 1) {
                            out += ",(";
                            wrap_open = true;
                        }
                        // The first variable right after a bracketed sector expression
                        // carries no separator: DATA SAVE DC OPEN T(700)A¤
                        else if (prev_oper == 0xD0 && closed_group && is_value_list(verb_id)) { }
                        else if (verb_id == 0x061E && prev_oper == 0xE0) { }  // LABEL A¤()3
                        else if (verb_id != 0x0602                       // not MAT REDIM
                         && verb_id != 0x80 && verb_id != 0x2A && verb_id != 0x2D)  // not SAVE DC / SAVE / LOAD
                            out += ',';
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
                        // When tables alone could not tell an array from a string scalar,
                        // an unambiguous index list does. Lists of receiving variables
                        // hold no constants, so a constant right after a variable always
                        // means indexing. The left part of LET and the first argument of
                        // STR( are ambiguous and give no evidence.
                        // Ambiguous places give no evidence: the left part of LET, the
                        // first argument of STR( and the target of CONVERT, which is
                        // followed by its format with an implicit comma just the same
                        if (all_vars[oper_id].shape_unknown && !is_let_left
                            && prev_oper != 0xE1 && prev_oper != 0xD1
                            && p < code.size() && opens_index(code[p])) {
                            all_vars[oper_id].is_array = true;
                            all_vars[oper_id].shape_unknown = false;
                        }
                        out += var_ref(oper_id, false);
                        is_operand = all_vars[oper_id].is_array;
                        // An index list is closed by its own D0, just like a parenthesis
                        if (is_operand) impl_stack.push_back({0,0,false});
                        break;
                    }
                case OperClass::Operand:
                    {
                        switch (oper_id) {
                        case 0xDE: // 1 byte binary constant
                            {
                                const uint8_t v = code[p++];
                                out += int_to_hex(v);
                                is_operand = false;
                                break;
                            }
                        case 0xE0: // Array reference
                            {
                                const uint8_t var_id = code[p++];
                                // A whole-array reference proves the variable is an array
                                all_vars[var_id].is_array = true;
                                all_vars[var_id].shape_unknown = false;
                                out += var_ref(var_id, false);
                                // MAT and MAT REDIM name the array without "()"
                                if (verb_id != 0x0602 && verb_id != 0x0601) out += "()";
                                // It is a complete value, so what follows is either an
                                // operation or the next item of a list
                                is_operand = false;
                                break;
                            }
                        case 0xE2: // HEX literal
                            {
                                const uint8_t hex_len = code[p++];
                                out += "HEX(";
                                for (int i=0; i<hex_len; i++) out += int_to_hex(code[p++]);
                                out += ')';
                                is_operand = false;
                                break;
                            }
                        case 0xE3: // String literal in quotes: <len> + KOI8 chars
                        case 0xE4: // The same in apostrophes - a lower case literal
                            {
                                // The image of CONVERT is already inside parentheses
                                const bool bare = (verb_id == 0x47 && wrap_open);
                                const char q = (oper_id == 0xE3) ? '"' : '\'';
                                const uint8_t str_len = code[p++];
                                if (!bare) out += q;
                                for (int i=0; i<str_len; i++) {
                                    out += (*cm.charmap)[code[p++]];
                                }
                                if (!bare) out += q;
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
                                break;
                            }
                        case 0xE5: // <descriptor> + BCD digits
                        case 0xE6: // the same + one more byte holding the exponent
                            {
                                out += decode_number(oper_id == 0xE6);
                                is_operand = false;
                                break;
                            }
                        case 0xDC: // device address prefix: "/" and then DE <byte>
                            {
                                out += '/';
                                if (p + 1 < code.size() && code[p] == 0xDE) {
                                    p++;
                                    out += int_to_hex(code[p++]);
                                }
                                is_operand = false;
                                break;
                            }
                        default:
                            // Everything else prints its own text (#PI and the like)
                            out += oper;
                            is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                            break;
                        }
                        break;
                    }
                case OperClass::Operation:
                    {
                        // A unary minus is legal where an operand is expected, and
                        // MAT SEARCH puts a comparison there on purpose
                        if (is_operand && verb_id!=0x060A && oper_id!=0xE9) out += "?OPERATION?";

                        const bool val_second = (oper_id == 0xDE && is_val_func
                                                && p < code.size() && code[p] == 0xDB);
                        if (closes_implicit_paren(oper_id) && !val_second
                            && !(is_pos_func && conditions(oper_id))) {
                            // We close the functions of this level, but not for the first
                            // condition in POS(), which takes a whole comparison
                            close_implicit(out);
                        }


                        if (is_pos_func && conditions(oper_id)) is_pos_func = false;

                        if (oper_id == 0xD9 && is_let_left) is_let_left = false; // "=" cancels processing of the left part of LETs

                        const bool sp = add_spaces(oper_id);
                        out += (sp?" ":"") + std::string(is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation) + (sp?" ":"");
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        break;
                    }
                case OperClass::Closer:
                    {
                        // Only the functions opened inside this parenthesis end here
                        close_implicit(out);
                        closed_group = false;
                        if (impl_stack.size() > 1) {
                            closed_group = impl_stack.back().group;
                            impl_stack.pop_back();
                        }
                        out += std::string(is_operand ? Iskra226_operations[oper_id-0xC0].as_operand : Iskra226_operations[oper_id-0xC0].as_operation);
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        break;
                    }
                case OperClass::VerbPart:
                    {
                        if (closes_implicit_paren(oper_id)) close_implicit(out);
                        // D6 in the operand slot of DATA SAVE DC / SAVE DC prints as "¤",
                        // and D2 there is the "T" parameter; both glue to the device spec
                        if (is_operand && oper_id == 0xD6
                            && (verb_id == 0x76 || verb_id == 0x80 || verb_id == 0x6E || verb_id == 0x6F)) {
                            out += "¤";
                            is_operand = true;
                            break;
                        }
                        if (is_operand && oper_id == 0xD2) {
                            out += "T";
                            is_operand = true;
                            break;
                        }
                        // BEG/END are values, so an operation may follow (SCRATCH DISK ...END=1000)
                        if (is_operand && (oper_id == 0xD6 || oper_id == 0xD7) && verb_id != 0x0600) {
                            if (!out.empty() && out.back()!=' ') out += ' ';
                            out += (oper_id == 0xD6) ? "BEG" : "END";
                            is_operand = false;
                            break;
                        }
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
                        default:
                            break;
                        }
                        is_operand = Iskra226_operations[oper_id-0xC0].next_is_operand;
                        break;
                    }
                case OperClass::ImplFunc:
                case OperClass::Function:
                    {
                        if (oper_id == 0xEF && verb_id == 0x0601) {
                            // Inside a MAT assignment EF means ZER, not VAL(
                            out += "ZER";
                            is_operand = false;
                            break;
                        }
                        if (oper_class == OperClass::ImplFunc) {
                            if (!impl_stack.empty()) {
                                // AT( is closed only at the end of the statement
                                if (oper_id == 0xD5) impl_stack.back().at++;
                                else impl_stack.back().impl++;
                            }
                        } else {
                            // An explicit parenthesis: everything opened inside it ends
                            // at its own D0
                            impl_stack.push_back({0,0,oper_id == 0xEB});
                        }
                        switch (oper_id) {
                        case 0xEC: // POS() needs special processing
                            {
                                is_pos_func = true;
                                break;
                            }
                        case 0xEF: // VAL() needs special processing
                            {
                                is_val_func = true;
                                break;
                            }
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
            impl_stack.assign(1, {0,0,false});
            is_pos_func = false;
            bin_target = false;
            wrap_open = false;
            closed_group = false;
            is_val_func = false;

            if (!is_first) { out += ':'; }
            uint16_t verb_id = code[p++];
            if (verb_id == 0x06) verb_id = (verb_id << 8) + code[p++];

            std::string verb = verb_by_id(verb_id);
            out +=  verb;

            const uint8_t oper_len = code[p++];
            if (oper_len > 0)
            {
                // No space after a verb whose name already ends with "(", and none
                // before the raw text of a comment - it keeps its own leading spaces
                if (!verb.empty() && verb.back() != '('
                    && verb_id != 0x3F && verb_id != 0x56) out +=  ' ';

                // Statements with a fixed-size head need that many operand bytes
                if ((oper_len < 2 && (verb_id==0x1E || verb_id==0x21 || verb_id==0x22 || verb_id==0x2F))
                 || (oper_len < 5 && (verb_id==0x25 || verb_id==0x27 || verb_id==0x3A))
                 || (oper_len < 2 && verb_id==0x29))
                {
                    // Damaged statement: show the bytes instead of guessing
                    out += "?" + toHexList(code) + "?";
                    return out;
                }

                switch (verb_id) {
                case 0x1E: //IF END THEN
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
                case 0x23: //GOSUB'
                    {
                        // A binary (not BCD) label, then the arguments
                        out += std::to_string(code[p++]);
                        if (p < code.size()) out += '(' + process_operands(verb_id) + ')';
                        break;
                    }
                case 0x29: //DATA
                    {
                        // The last two bytes are a link to the next DATA statement
                        if (code.size() >= 2) code.resize(code.size() - 2);
                        if (p < code.size()) out += process_operands(verb_id);
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
                case 0x3A: //DEFFN' with a literal text (key definition)
                    {
                        // A binary label, four bytes filled in at run time, then the body
                        const uint8_t gosub_id = code[p++];
                        out += std::to_string(gosub_id);
                        p += 4;
                        if (p < code.size()) {
                            const std::string args = process_operands(verb_id);
                            if (verb_id == 0x27) out += '(' + args + ')';
                            else out += args;
                        }
                        break;
                    }
                case 0x51: //RESTORE <expr>,<line>
                    {
                        // The line number is a raw two byte BCD, without a token
                        if (oper_len >= 3 && code[code.size()-3] == 0xDE) {
                            const unsigned keep = code.size() - 2;
                            const uint16_t line = FROM_BCD_BE_16(code, keep);
                            code.resize(keep);
                            out += process_operands(verb_id);
                            out += std::to_string(line);
                        } else {
                            out += process_operands(verb_id);
                        }
                        break;
                    }
                case 0x2A: //SAVE
                case 0x2D: //LOAD
                    {
                        // [DD] <name> <line>[,<line>...] - the line numbers are raw BCD
                        if (code[p] == 0xDD) p++;
                        if (p < code.size()) {
                            const uint8_t t = code[p];
                            if (!(t == 0xE0 || t == 0xE3 || t == 0xE4 || IS_VAR(t))) {
                                // the name is an expression, leave the whole thing alone
                                out += process_operands(verb_id);
                                break;
                            }
                            if (t == 0xE0 && p+1 < code.size()) { p++; out += var_ref(code[p++], false) + "()"; }
                            else if ((t == 0xE3 || t == 0xE4) && p+1 < code.size()) {
                                const char q = (t == 0xE3) ? '"' : '\'';
                                p++;
                                const uint8_t str_len = code[p++];
                                out += q;
                                for (int i=0; i<str_len && p<code.size(); i++) out += (*cm.charmap)[code[p++]];
                                out += q;
                            }
                            else if (IS_VAR(t)) { out += var_ref(t, false); p++; }
                        }
                        bool first_line = true;
                        while (p + 1 < code.size()) {
                            if (code[p] == 0xDE) p++;
                            if (p + 1 >= code.size()) break;
                            if (!first_line) out += ',';
                            first_line = false;
                            out += std::to_string(FROM_BCD_BE_16(code, p));
                            p += 2;
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
                        // ¤GIO '<microprogram>,<var>  or  ¤GIO /<addr>,(<microprogram>,<var>)
                        const uint8_t dev_id = code[p];
                        if (dev_id == 0xD5) {
                            out += '\'';
                            p++;
                            out += process_operands(verb_id);
                        } else if (dev_id == 0xDC && p+3 < code.size()) {
                            p += 2;                       // DC DE
                            out += '/' + int_to_hex(code[p++]);
                            if (p < code.size() && code[p] == 0xDE) p++;
                            out += ",(" + process_operands(verb_id) + ')';
                        } else {
                            out += process_operands(verb_id);
                        }
                        break;
                    }
                case 0x46: //DIM
                case 0x4E: //COM
                    {
                        // Operands are just variable indices, without separators.
                        // Everything else comes from the variable model built out of
                        // tables 2/3 and table 1.
                        bool first_var = true;
                        while (p < code.size()) {
                            const uint8_t var_id = code[p++];
                            if (!first_var) out += ',';
                            first_var = false;
                            out += var_decl(var_id);
                        }
                        break;
                    }
                case 0x48: // PACK(
                case 0x5D: // UNPACK(
                    {
                        // String literal, then other
                        const uint8_t next_part = code[p];
                        if (next_part == 0xE3) {
                            p++;
                            const uint8_t str_len = code[p++];
                            for (int i=0; i<str_len; i++) {
                                out += (*cm.charmap)[code[p++]];
                            }
                            out += ')';
                        }
                        out += process_operands(verb_id);
                        break;
                    }
                case 0x54: //SELECT
                    {
                        // A list of device group records separated by DE
                        bool first_rec = true;
                        while (p < code.size()) {
                            if (!first_rec) {
                                if (code[p] == 0xDE) p++;
                                out += ',';
                                if (p >= code.size()) break;
                            }
                            first_rec = false;
                            const uint8_t dev_class = code[p++];
                            switch (dev_class) {
                            case 0x00: // #<file><address>[<drive>]
                                {
                                    if (p + 1 >= code.size()) break;
                                    const uint16_t n = FROM_BE_16(code, p); p+=2;
                                    out += "#" + int_to_hex(n, false);
                                    // the drive letter is optional
                                    if (p < code.size() && code[p] != 0xDE) out += device_letter(code[p++]);
                                    break;
                                }
                            case 0x05: // P<digit> - a pause after every line
                                {
                                    out += "P";
                                    if (p < code.size() && code[p] != 0xDE) out += std::to_string(code[p++]);
                                    break;
                                }
                            case 0x06: // presumably LIST
                            case 0x07: // PRINT
                            case 0x08: // presumably PLOT
                            case 0x0C: // presumably CO
                                {
                                    out += (dev_class == 0x07) ? "PRINT" :
                                           (dev_class == 0x06) ? "LIST"  :
                                           (dev_class == 0x08) ? "PLOT"  : "CO";
                                    if (p < code.size() && code[p] != 0xDE) out += int_to_hex(code[p++]);
                                    if (p + 2 < code.size() && code[p] == 0xEB) {
                                        p++;
                                        const uint16_t n = FROM_BE_16(code, p); p+=2;
                                        out += "(" + std::to_string(n) + ")";
                                    }
                                    break;
                                }
                            case 0x0A: // DISK<address><drive>
                                {
                                    if (p >= code.size()) break;
                                    out += "DISK" + int_to_hex(code[p++]);
                                    if (p < code.size() && code[p] != 0xDE) out += device_letter(code[p++]);
                                    break;
                                }
                            default:
                                {
                                    out += "?" + int_to_hex(dev_class) + "?";
                                    p = code.size();
                                    break;
                                }
                            }
                        }
                        break;
                    }
                case 0x6E: //DATA SAVE BA
                case 0x6F: //DATA SAVE DA
                case 0x70: //DATA LOAD BA
                case 0x71: //DATA LOAD DA
                case 0x75: //DATA LOAD DC OPEN
                case 0x78: //DATA SAVE DC OPEN
                case 0x7B: //LIMITS
                case 0x7C: //LIST DC
                case 0x7D: //LOAD DC
                case 0x80: //SAVE DC
                case 0x81: //SCRATCH
                case 0x83: //VERIFY
                    {
                        // The first operand byte selects a device or an access mode,
                        // unless the statement starts with a device address instead
                        if (code[p] <= 2) out += device_letter(code[p++]);
                        if (p < code.size()) out += process_operands(verb_id);
                        break;
                    }
                case 0x82: //SCRATCH DISK
                    {
                        // <device> <06 = LS> = <n> , END = <n>
                        out += device_letter(code[p++]);
                        bool ls = false;
                        if (p < code.size() && code[p] == 0x06) { out += "LS"; p++; ls = true; }
                        if (p < code.size()) out += process_operands(verb_id, !ls);
                        break;
                    }

                case 0x64: //INIT
                    {
                        // INIT (<value>)<variable list> - the parentheses are not encoded
                        const uint8_t first_oper = code[p];
                        if (first_oper == 0xDE) {
                            p++;
                            const uint8_t v = code[p++];
                            out += "(" + int_to_hex(v) + ")";
                        } else if (first_oper == 0xE3 || first_oper == 0xE4) {
                            const char q = (first_oper == 0xE3) ? '"' : '\'';
                            p++;
                            const uint8_t str_len = code[p++];
                            out += '(';
                            out += q;
                            for (int i=0; i<str_len && p<code.size(); i++) out += (*cm.charmap)[code[p++]];
                            out += q;
                            out += ')';
                        }
                        out += process_operands(verb_id);
                        break;
                    }

                case 0x43: // AND(
                case 0x61: // OR(
                case 0x62: // XOR(
                    {
                        // The whole operand list is inside the parenthesis of the verb,
                        // and the closing one is not encoded
                        out += process_operands(verb_id);
                        close_all_implicit(out);
                        out += ')';
                        break;
                    }
                case 0x4B: // BIN(
                    {
                        bin_target = true;
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
            close_all_implicit(out);
            if (wrap_open) out += ')';

            return out;
        }
    };

    // If the rest of the current sector is zero, the next record has been moved
    // to the beginning of the following one
    static unsigned skip_padding(const BYTES & code, unsigned p)
    {
        unsigned b = ((p / SECTOR_DATA) + 1) * SECTOR_DATA;
        if (b > code.size()) b = code.size();
        if (b <= p) return p;
        for (unsigned i = p; i < b; i++) if (code[i] != 0) return p;
        return b;
    }

    // A record is <line number: 2 bytes BCD> <len: 1 byte> <body>, and the byte
    // right after the body must be the FE separator
    static bool is_record_start(const BYTES & code, unsigned p)
    {
        p = skip_padding(code, p);
        if (p + 3 > code.size()) return false;
        for (unsigned i = 0; i < 2; i++)
            if ((code[p+i] >> 4) > 9 || (code[p+i] & 0x0F) > 9) return false;
        const unsigned len = code[p+2];
        if (len < 1) return false;
        const unsigned e = p + 2 + len;
        return (e < code.size()) && (code[e] == 0xFE);
    }

    std::string ViewerBASIC_Iskra226::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        std::string out;
        cm = init_charmap(cm_name);

        if (data.size() < 256*2) return "NOT A BASIC";
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
                out += "\nUNKNOWN BLOCK SIGNATURE AT " + int_to_hex(part-data.data()) + '\n';
                part += 256;
            }
        } else {
            out += "TOKENIZED BASIC\n\n";

            BYTES code;

            // Collecting parts of the code to a single stream removing header bytes.
            // Every sector contributes exactly SECTOR_DATA bytes, trailing zeroes
            // included: the alignment rule below relies on the sector boundaries
            // staying at multiples of SECTOR_DATA.
            const unsigned char * part = &data[256];
            const unsigned char * end = data.data() + data.size();
            while (part + 256 <= end)
            {
                const uint8_t b1 = part[0];
                const uint8_t b2 = part[1];
                if (b1 == 0x1C) break;
                if (b2 != 0x80) break;
                code.insert(code.end(), part + 2, part + 256);
                part += 256;
            }
            if (code.size() < 8) return "NOT A BASIC";

            const uint16_t L1 = FROM_BE_16(code, 0);
            const uint16_t L2 = FROM_BE_16(code, 2);
            const uint16_t L3 = FROM_BE_16(code, 4);

            out += "L1: 0x" + int_to_hex(L1) + '\n';
            out += "L2: 0x" + int_to_hex(L2) + '\n';
            out += "L3: 0x" + int_to_hex(L3) + '\n';

            out += '\n';

            if (static_cast<unsigned>(6) + L1 + L2 + L3 > code.size()) return "NOT A BASIC";

            unsigned prog_start = 6 + L1 + L2 + L3;

            // Some programs carry a four byte field of unknown purpose between the
            // tables and the first record. Recognised by validating the record itself.
            if (!is_record_start(code, prog_start) && is_record_start(code, prog_start + 4)) {
                out += "EXTRA FIELD: " + toHexList(BYTES(code.begin() + prog_start, code.begin() + prog_start + 4)) + '\n';
                prog_start += 4;
            }

            out += "PROGRAM AT: 0x" + int_to_hex(0x100 + 2 + prog_start) + "\n\n";

            Iskra226Parser parser(cm);
            parser.build_vars(code, L1, L2, L3);

            // The first pass only lets the parser learn which variables are arrays
            // (whole-array references appear anywhere in the program), the second one
            // produces the listing.
            for (int pass = 0; pass < 2; pass++) {
                std::string listing;
                unsigned p = prog_start;

                while (p + 3 <= code.size()) {
                    p = skip_padding(code, p);
                    if (p + 3 > code.size()) break;

                    const uint16_t line_num = FROM_BCD_BE_16(code, p);
                    const uint8_t line_len = code[p+2];
                    if (line_len < 1) break;

                    p += 3;

                    listing += pad_number(line_num, 4, '0') + " ";

                    const unsigned line_start = p;
                    const unsigned line_end = line_start + line_len - 1;

                    while (p < line_end && p < code.size()) {
                        const unsigned operator_start = p;
                        uint16_t verb_id = code[p++];

                        if (verb_id == 0x06 && p < code.size()) verb_id = (verb_id << 8) + code[p++]; // Extended codes
                        if (p >= code.size()) break;

                        const unsigned ld = (verb_id<0x100 ? 1 : 2);
                        const unsigned operator_len = code[p] + ld + 1;
                        if (operator_start + operator_len > code.size()) { p = code.size(); break; }
                        const auto operator_code = BYTES(code.begin() + operator_start, code.begin() + operator_start + operator_len);
                        listing += parser.parse(operator_code, p==line_start+ld);
                        p += operator_len - ld;
                    }

                    listing += "\n";

                    if (p >= code.size()) break;
                    if (code[p] != 0xFE) {
                        listing += "Delimiter 0xFE not found, finishing\n";
                        break;
                    }
                    p+=1;
                }

                if (pass == 1) out += listing;
            }

        }


        return escapeHtml(out);
    }

}
