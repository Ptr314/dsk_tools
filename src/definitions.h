#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>
#include <string>
#include <vector>

namespace dsk_tools {

    #define FDD_LOAD_OK                 0
    #define FDD_LOAD_ERROR              1
    #define FDD_LOAD_SIZE_MISMATCH      2
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
    #define FDD_WRITE_INCORECT_TEMPLATE 4
    #define FDD_WRITE_INCORECT_SOURCE   5

    #define FDD_DETECT_OK               0
    #define FDD_DETECT_ERROR            1

    #define PREFERRED_BINARY            0
    #define PREFERRED_TEXT              1


    typedef std::vector<uint8_t> BYTES;

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

    static const int agat_140_raw2logic[16] = {
         0, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 15
     };

}

#endif // DEFINITIONS_H
