// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Some constants definitions

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "bit_enums.h"

namespace dsk_tools {

    #define FDD_LOAD_OK                 0
    #define FDD_LOAD_ERROR              1
    #define FDD_LOAD_SIZE_MISMATCH      2
    #define FDD_LOAD_PARAMS_MISMATCH    4
    #define FDD_LOAD_INCORRECT_FILE     5
    #define FDD_LOAD_FILE_CORRUPT       6
    #define FDD_LOAD_DATA_CORRUPT       7

    #define FDD_OPEN_OK                 0
    #define FDD_OPEN_NOT_LOADED         1
    #define FDD_OPEN_BAD_FORMAT         2

    #define FDD_OP_OK                   0
    #define FDD_OP_ERROR                1
    #define FDD_OP_NOT_OPEN             2

    #define FDD_WRITE_OK                0
    #define FDD_WRITE_ERROR             1
    #define FDD_WRITE_UNSUPPORTED       2
    #define FDD_WRITE_ERROR_READING     3
    #define FDD_WRITE_INCORECT_TEMPLATE 4
    #define FDD_WRITE_INCORECT_SOURCE   5

    #define FDD_DIR_OK                  0
    #define FDD_DIR_ERROR               1
    #define FDD_DIR_ERROR_SPACE         2
    #define FDD_DIR_NOT_EMPTY           3

    #define FILE_DELETE_OK              0
    #define FILE_DELETE_ERROR           1

    #define FILE_ADD_OK                 0
    #define FILE_ADD_ERROR              1
    #define FILE_ADD_ERROR_IO           2
    #define FILE_ADD_ERROR_SPACE        3

    #define FILE_RENAME_OK              0
    #define FILE_RENAME_ERROR           1

    #define FILE_METADATA_OK            0
    #define FILE_METADATA_ERROR         1

    #define FDD_DETECT_OK               0
    #define FDD_DETECT_ERROR            1

    #define PREFERRED_BINARY            0
    #define PREFERRED_TEXT              1
    #define PREFERRED_AGATBASIC         2
    #define PREFERRED_ABS               3
    #define PREFERRED_MBASIC            4

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
        std::vector<uint32_t>   position;
    };

    enum class FSCaps : unsigned int {
        None        = 0,
        Dirs        = 1 << 0,       // Directories present
        Protect     = 1 << 1,       // File protection allowed
        Types       = 1 << 2,       // Files have types
        Delete      = 1 << 3,       // Files can be deleted
        Add         = 1 << 4,       // Files can be added
        Rename      = 1 << 5,       // Files can be renamed
        All         = Dirs | Protect | Types | Delete | Add | Rename
    };

    ENABLE_ENUM_FLAG_OPERATORS(FSCaps);

    enum class FS {Host, DOS33, Sprite, CPM};
    enum class PreferredType {Binary, Text, AgatBASIC, AppleBASIC, MBASIC};

    struct UniversalFile {
        FS                      fs;             // Original filesystem type

        // Universal data
        std::string             name;           // Common name
        uint32_t                size;
        bool                    is_dir;
        bool                    is_protected;
        bool                    is_deleted;
        PreferredType           type;
        std::vector<uint8_t>    data;

        // FS-specific data
        std::vector<uint8_t>    original_name;
        uint32_t                attributes;
        std::vector<uint8_t>    metadata;
        std::vector<uint32_t>   position;
    };

    typedef std::vector<UniversalFile> Files;

    enum class ErrorCode {
        Ok = 0,

        // Load errors (FDD_LOAD_*)
        LoadError,
        LoadSizeMismatch,
        LoadParamsMismatch,
        LoadIncorrectFile,
        LoadFileCorrupt,
        LoadDataCorrupt,

        // Open errors (FDD_OPEN_*)
        OpenNotLoaded,
        OpenBadFormat,

        // Operation errors (FDD_OP_*)
        OperationError,
        OperationNotOpen,

        // Write errors (FDD_WRITE_*)
        WriteError,
        WriteUnsupported,
        WriteErrorReading,
        WriteIncorrectTemplate,
        WriteIncorrectSource,

        // Directory errors (FDD_DIR_*)
        DirError,
        DirErrorSpace,
        DirNotEmpty,

        // File operation errors (FILE_*)
        FileDeleteError,
        FileAddError,
        FileAddErrorIO,
        FileAddErrorSpace,
        FileRenameError,
        FileMetadataError,

        // Detection errors (FDD_DETECT_*)
        DetectError
    };

    struct Result {
        ErrorCode code;
        std::string message;   // empty if Ok
        static Result ok() {return Result{ ErrorCode::Ok, "" };}
        static Result error(ErrorCode c, std::string msg) {return Result{ c, std::move(msg) };}
        bool isOk() const {return code == ErrorCode::Ok;}
        operator bool() const { return isOk(); } // convenience: allow `if (res)` syntax
    };

    static const int agat_140_raw2logic[16] = {
        0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15
    };

    static const int agat_140_cpm2dos[16] = {
        0,6,12,3,9,15,14,5,11,2,8,7,13,4,10,1
    };

    static const int agat_140_cpm2prodos[16] = {
        0,9,3,12,6,15,1,10,4,13,7,8,2,11,5,14
    };

    static const uint16_t agat_MFM_tab[]=
        {
            0x5555, 0x5595, 0x5525, 0x55A5, 0x5549, 0x5589, 0x5529, 0x55A9,
            0x5552, 0x5592, 0x5522, 0x55A2, 0x554A, 0x558A, 0x552A, 0x55AA,
            0x9554, 0x9594, 0x9524, 0x95A4, 0x9548, 0x9588, 0x9528, 0x95A8,
            0x9552, 0x9592, 0x9522, 0x95A2, 0x954A, 0x958A, 0x952A, 0x95AA,
            0x2555, 0x2595, 0x2525, 0x25A5, 0x2549, 0x2589, 0x2529, 0x25A9,
            0x2552, 0x2592, 0x2522, 0x25A2, 0x254A, 0x258A, 0x252A, 0x25AA,
            0xA554, 0xA594, 0xA524, 0xA5A4, 0xA548, 0xA588, 0xA528, 0xA5A8,
            0xA552, 0xA592, 0xA522, 0xA5A2, 0xA54A, 0xA58A, 0xA52A, 0xA5AA,
            0x4955, 0x4995, 0x4925, 0x49A5, 0x4949, 0x4989, 0x4929, 0x49A9,
            0x4952, 0x4992, 0x4922, 0x49A2, 0x494A, 0x498A, 0x492A, 0x49AA,
            0x8954, 0x8994, 0x8924, 0x89A4, 0x8948, 0x8988, 0x8928, 0x89A8,
            0x8952, 0x8992, 0x8922, 0x89A2, 0x894A, 0x898A, 0x892A, 0x89AA,
            0x2955, 0x2995, 0x2925, 0x29A5, 0x2949, 0x2989, 0x2929, 0x29A9,
            0x2952, 0x2992, 0x2922, 0x29A2, 0x294A, 0x298A, 0x292A, 0x29AA,
            0xA954, 0xA994, 0xA924, 0xA9A4, 0xA948, 0xA988, 0xA928, 0xA9A8,
            0xA952, 0xA992, 0xA922, 0xA9A2, 0xA94A, 0xA98A, 0xA92A, 0xA9AA,
            0x5255, 0x5295, 0x5225, 0x52A5, 0x5249, 0x5289, 0x5229, 0x52A9,
            0x5252, 0x5292, 0x5222, 0x52A2, 0x524A, 0x528A, 0x522A, 0x52AA,
            0x9254, 0x9294, 0x9224, 0x92A4, 0x9248, 0x9288, 0x9228, 0x92A8,
            0x9252, 0x9292, 0x9222, 0x92A2, 0x924A, 0x928A, 0x922A, 0x92AA,
            0x2255, 0x2295, 0x2225, 0x22A5, 0x2249, 0x2289, 0x2229, 0x22A9,
            0x2252, 0x2292, 0x2222, 0x22A2, 0x224A, 0x228A, 0x222A, 0x22AA,
            0xA254, 0xA294, 0xA224, 0xA2A4, 0xA248, 0xA288, 0xA228, 0xA2A8,
            0xA252, 0xA292, 0xA222, 0xA2A2, 0xA24A, 0xA28A, 0xA22A, 0xA2AA,
            0x4A55, 0x4A95, 0x4A25, 0x4AA5, 0x4A49, 0x4A89, 0x4A29, 0x4AA9,
            0x4A52, 0x4A92, 0x4A22, 0x4AA2, 0x4A4A, 0x4A8A, 0x4A2A, 0x4AAA,
            0x8A54, 0x8A94, 0x8A24, 0x8AA4, 0x8A48, 0x8A88, 0x8A28, 0x8AA8,
            0x8A52, 0x8A92, 0x8A22, 0x8AA2, 0x8A4A, 0x8A8A, 0x8A2A, 0x8AAA,
            0x2A55, 0x2A95, 0x2A25, 0x2AA5, 0x2A49, 0x2A89, 0x2A29, 0x2AA9,
            0x2A52, 0x2A92, 0x2A22, 0x2AA2, 0x2A4A, 0x2A8A, 0x2A2A, 0x2AAA,
            0xAA54, 0xAA94, 0xAA24, 0xAAA4, 0xAA48, 0xAA88, 0xAA28, 0xAAA8,
            0xAA52, 0xAA92, 0xAA22, 0xAAA2, 0xAA4A, 0xAA8A, 0xAA2A, 0xAAAA,
            0x5455, 0x5495, 0x5425, 0x54A5, 0x5449, 0x5489, 0x5429, 0x54A9,
            0x5452, 0x5492, 0x5422, 0x54A2, 0x544A, 0x548A, 0x542A, 0x54AA,
            0x9454, 0x9494, 0x9424, 0x94A4, 0x9448, 0x9488, 0x9428, 0x94A8,
            0x9452, 0x9492, 0x9422, 0x94A2, 0x944A, 0x948A, 0x942A, 0x94AA,
            0x2455, 0x2495, 0x2425, 0x24A5, 0x2449, 0x2489, 0x2429, 0x24A9,
            0x2452, 0x2492, 0x2422, 0x24A2, 0x244A, 0x248A, 0x242A, 0x24AA,
            0xA454, 0xA494, 0xA424, 0xA4A4, 0xA448, 0xA488, 0xA428, 0xA4A8,
            0xA452, 0xA492, 0xA422, 0xA4A2, 0xA44A, 0xA48A, 0xA42A, 0xA4AA,
            0x4855, 0x4895, 0x4825, 0x48A5, 0x4849, 0x4889, 0x4829, 0x48A9,
            0x4852, 0x4892, 0x4822, 0x48A2, 0x484A, 0x488A, 0x482A, 0x48AA,
            0x8854, 0x8894, 0x8824, 0x88A4, 0x8848, 0x8888, 0x8828, 0x88A8,
            0x8852, 0x8892, 0x8822, 0x88A2, 0x884A, 0x888A, 0x882A, 0x88AA,
            0x2855, 0x2895, 0x2825, 0x28A5, 0x2849, 0x2889, 0x2829, 0x28A9,
            0x2852, 0x2892, 0x2822, 0x28A2, 0x284A, 0x288A, 0x282A, 0x28AA,
            0xA854, 0xA894, 0xA824, 0xA8A4, 0xA848, 0xA888, 0xA828, 0xA8A8,
            0xA852, 0xA892, 0xA822, 0xA8A2, 0xA84A, 0xA88A, 0xA82A, 0xA8AA,
            0x5255, 0x5295, 0x5225, 0x52A5, 0x5249, 0x5289, 0x5229, 0x52A9,
            0x5252, 0x5292, 0x5222, 0x52A2, 0x524A, 0x528A, 0x522A, 0x52AA,
            0x9254, 0x9294, 0x9224, 0x92A4, 0x9248, 0x9288, 0x9228, 0x92A8,
            0x9252, 0x9292, 0x9222, 0x92A2, 0x924A, 0x928A, 0x922A, 0x92AA,
            0x2255, 0x2295, 0x2225, 0x22A5, 0x2249, 0x2289, 0x2229, 0x22A9,
            0x2252, 0x2292, 0x2222, 0x22A2, 0x224A, 0x228A, 0x222A, 0x22AA,
            0xA254, 0xA294, 0xA224, 0xA2A4, 0xA248, 0xA288, 0xA228, 0xA2A8,
            0xA252, 0xA292, 0xA222, 0xA2A2, 0xA24A, 0xA28A, 0xA22A, 0xA2AA,
            0x4A55, 0x4A95, 0x4A25, 0x4AA5, 0x4A49, 0x4A89, 0x4A29, 0x4AA9,
            0x4A52, 0x4A92, 0x4A22, 0x4AA2, 0x4A4A, 0x4A8A, 0x4A2A, 0x4AAA,
            0x8A54, 0x8A94, 0x8A24, 0x8AA4, 0x8A48, 0x8A88, 0x8A28, 0x8AA8,
            0x8A52, 0x8A92, 0x8A22, 0x8AA2, 0x8A4A, 0x8A8A, 0x8A2A, 0x8AAA,
            0x2A55, 0x2A95, 0x2A25, 0x2AA5, 0x2A49, 0x2A89, 0x2A29, 0x2AA9,
            0x2A52, 0x2A92, 0x2A22, 0x2AA2, 0x2A4A, 0x2A8A, 0x2A2A, 0x2AAA,
            0xAA54, 0xAA94, 0xAA24, 0xAAA4, 0xAA48, 0xAA88, 0xAA28, 0xAAA8,
            0xAA52, 0xAA92, 0xAA22, 0xAAA2, 0xAA4A, 0xAA8A, 0xAA2A, 0xAAAA
    };

    static const uint16_t agat_MFM_decode_tab[128] =
        {
            0x0, 0x8, 0x0, 0x8, 0x4, 0xc, 0x4, 0xc, 0x0, 0x8, 0x0, 0x8, 0x4, 0xc, 0x4, 0xc,
            0x2, 0xa, 0x2, 0xa, 0x6, 0xe, 0x6, 0xe, 0x2, 0xa, 0x2, 0xa, 0x6, 0xe, 0x6, 0xe,
            0x0, 0x8, 0x0, 0x8, 0x4, 0xc, 0x4, 0xc, 0x0, 0x8, 0x0, 0x8, 0x4, 0xc, 0x4, 0xc,
            0x2, 0xa, 0x2, 0xa, 0x6, 0xe, 0x6, 0xe, 0x2, 0xa, 0x2, 0xa, 0x6, 0xe, 0x6, 0xe,
            0x1, 0x9, 0x1, 0x9, 0x5, 0xd, 0x5, 0xd, 0x1, 0x9, 0x1, 0x9, 0x5, 0xd, 0x5, 0xd,
            0x3, 0xb, 0x3, 0xb, 0x7, 0xf, 0x7, 0xf, 0x3, 0xb, 0x3, 0xb, 0x7, 0xf, 0x7, 0xf,
            0x1, 0x9, 0x1, 0x9, 0x5, 0xd, 0x5, 0xd, 0x1, 0x9, 0x1, 0x9, 0x5, 0xd, 0x5, 0xd,
            0x3, 0xb, 0x3, 0xb, 0x7, 0xf, 0x7, 0xf, 0x3, 0xb, 0x3, 0xb, 0x7, 0xf, 0x7, 0xf
    };

    #define HXC_HFE_BLOCK_SIZE  512

    #pragma pack(push, 1)

    // https://hxc2001.com/floppy_drive_emulator/HFE-file-format.html
    struct HXC_HFE_HEADER
    {
        uint8_t  HEADERSIGNATURE[8];        // "HXCPICFE" for HFEv1 and HFEv2, "HXCHFEV3" for HFEv3
        uint8_t  formatrevision;            // 0 for the HFEv1, 1 for the HFEv2. Reset to 0 for HFEv3.
        uint8_t  number_of_track;           // Number of track(s) in the file
        uint8_t  number_of_side;            // Number of valid side(s)
        uint8_t  track_encoding;            // Track Encoding mode
        uint16_t bitRate;                   // Bitrate in Kbit/s
        uint16_t floppyRPM;                 // Rotation per minute
        uint8_t  floppyinterfacemode;       // Floppy interface mode
        uint8_t  write_protected;           // Reserved
        uint16_t track_list_offset;         // Offset of the track list LUT in block of 512 bytes
        uint8_t  write_allowed;             // 0x00 : Write protected, 0xFF: Unprotected
        // v1.1 addition
        uint8_t  single_step;               // 0xFF : Single Step - 0x00 Double Step mode
        uint8_t  track0s0_altencoding;      // 0x00 : Use an alternate track_encoding for track 0 Side 0
        uint8_t  track0s0_encoding;         // alternate track_encoding for track 0 Side 0
        uint8_t  track0s1_altencoding;      // 0x00 : Use an alternate track_encoding for track 0 Side 1
        uint8_t  track0s1_encoding;         // alternate track_encoding for track 0 Side 1
    };

    struct HXC_HFE_TRACK
    {
        uint16_t	offset;                 // Track data offset in block of 512 bytes (Ex: 2 = 0x400)
        uint16_t	track_len;              // Length of the track data in byte.
    };

    #pragma pack(pop)

    // http://forum.agatcomp.ru//viewtopic.php?id=193
    // can't use vectors because of non-POD error
    constexpr std::array<const char*, 8> agat_file_types = {"T", "I", "A", "B", "S", "П", "К", "Д"};

    // https://agatcomp.ru/agat/PCutils/EXIF.shtml
    constexpr std::array<const char*, 16> agat_vr_mode_low = {
        "{$AGAT_VR_AGAT_GMODES}",           // 0
        "{$AGAT_VR_AGAT_TMODES}",           // 1
        "", "", "", "", "", "", "", "",     // 2-9
        "{$AGAT_VR_A2_MODES}",              // A
        "", "",                             // B-C
        "{$AGAT_VR_GIGA_MODES}",            // D
        "", ""                              // E-F
    };
    constexpr std::array<const char*, 16> agat_vr_mode_high_agat_gr = {
        "",
        "256х256 МГВР",
        "", "",
        "64х64 ЦГНР",
        "128х128 ЦГСР",
        "256х256 ЦГВР",
        "512х256 МГДП",
        "128х256 16 цветов",
        "280х192 HGR",
        "", "", "", "", "", ""
    };
    constexpr std::array<const char*, 16> agat_vr_mode_high_agat_tx = {
        "", "",
        "Т32 Ц",
        "Т64 М",
        "Т64 Ц (Turbo Agat)",
        "", "", "", "", "",
        "Т32 ЦЦ",
        "",
        "Т64 ЦЦ (Turbo Agat)",
        "", "", ""
    };
    constexpr std::array<const char*, 16> agat_vr_mode_high_apple = {
        "", "", "", "", "", "", "", "", "",
        "280х192 HiRes",
        "T40",
        "T80",
        "40х48 LoRes",
        "80x48 Double LoRes",
        "140x192 Double Hires color",
        "560x192 Double Hires b/w"
    };

    constexpr std::array<const char*, 16> agat_vr_font = {
        "", "", "", "", "", "", "",
        "Agat-7 standard (128 characters)",
        "Agat-7 extended (256 characters)",
        "Agat-9 standard",
        "", "", "", "", "",
        "Custom Font"
    };

}
