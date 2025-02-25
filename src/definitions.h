#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>
#include <string>
#include <vector>

namespace dsk_tools {

    #define FDD_LOAD_OK                 0
    #define FDD_LOAD_ERROR              1
    #define FDD_LOAD_SIZE_SMALLER       2
    #define FDD_LOAD_SIZE_LARGER        3
    #define FDD_LOAD_PARAMS_MISMATCH    4
    #define FDD_LOAD_INCORRECT_FILE     5
    #define FDD_LOAD_FILE_CORRUPT       6

    #define FDD_OPEN_OK                 0
    #define FDD_OPEN_NOT_LOADED         1
    #define FDD_OPEN_BAD_FORMAT         2

    #define FDD_OP_OK                   0
    #define FDD_OP_NOT_OPEN             1

    #define FILE_PROTECTION             1
    #define FILE_TYPE                   2
    #define FILE_DELETE                 4
    #define FILE_DIRS                   8

    #define FDD_WRITE_OK                0
    #define FDD_WRITE_ERROR             1
    #define FDD_WRITE_UNSUPPORTED       2
    #define FDD_WRITE_ERROR_READING     3

    #define FDD_DETECT_OK               0
    #define FDD_DETECT_ERROR            1

    #define PREFERRED_BINARY            0
    #define PREFERRED_TEXT              1


    #define BYTES   std::vector<uint8_t>

    struct fileData
    {
        uint8_t                 original_name[100];
        int                     original_name_length;
        std::string             name;
        std::string             type_str;
        std::string             type_str_short;
        bool                    is_protected;
        bool                    is_deleted;
        bool                    is_dir;
        uint32_t                attributes;
        uint32_t                size;
        int                     preferred_type;
        std::vector<uint8_t>    metadata;
    };

    // TODO: http://forum.agatcomp.ru//viewtopic.php?id=378
    static const std::string agat_charmap[256] =  {
        " ", "▃", "▅", "▇", "⋁", "╗", "∀", "∃", "←", "╭", "╮", "╯", "╰", "⬐", "↘", "ё",
        "┌", "┬", "┐", "├", "┼", "→", "↗", "↙", "↖", "↑", "↓", "─", "┤", "└", "┴", "┘",
        " ", "α", "β", "γ", "ε", "λ", "μ", "π", "ρ", "τ", "φ", "ψ", "ω", "∑", "Δ", "÷",
        "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", "⁻", "≡", "≤", "≠", "≥", "±",
        "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
        "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "│", "}", "~", "█",
        "ю", "а", "б", "ц", "д", "е", "ф", "г", "х", "и", "й", "к", "л", "м", "н", "о",
        "п", "я", "р", "с", "т", "у", "ж", "в", "ь", "ы", "з", "ш", "э", "щ", "ч", "ъ",

        ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".",
        ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", ".", "∙", "Ё",
        " ", "!", """","#", "¤", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
        "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_",
        "Ю", "А", "Б", "Ц", "Д", "Е", "Ф", "Г", "Х", "И", "Й", "К", "Л", "М", "Н", "О",
        "П", "Я", "Р", "С", "Т", "У", "Ж", "В", "Ь", "Ы", "З", "Ш", "Э", "Щ", "Ч", "Ъ"
    };
}

#endif // DEFINITIONS_H
