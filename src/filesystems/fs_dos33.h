// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the Apple DOS 3.3 filesystem

#ifndef FS_DOS33_H
#define FS_DOS33_H

#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    struct Apple_DOS_VTOC
    {
        uint8_t     _not_used_00;        // 00
        uint8_t     catalog_track;       // 01
        uint8_t     catalog_sector;      // 02
        uint8_t     dos_release;         // 03
        uint8_t     _not_used_04[2];     // 04-05
        uint8_t     volume_id;           // 06
        uint8_t     _not_used_07[0x20];  // 07-26
        uint8_t     pairs_on_sector;     // 27
        uint8_t     _not_used_28[8];     // 28-2F
        uint8_t     last_track;          // 30
        uint8_t     direction;           // 31
        uint8_t     _not_used_32[2];     // 32-33
        uint8_t     tracks_total;        // 34
        uint8_t     sectors_on_track;    // 35
        uint16_t    bytes_per_sector;    // 36-37
        uint32_t    free_sectors[50];    // 38-FF
    };

    struct Agat_VTOC
    {
        uint8_t     _not_used_00;        // 00
        uint8_t     catalog_track;       // 01
        uint8_t     catalog_sector;      // 02
        uint8_t     dos_release;         // 03
        uint8_t     _not_used_04[2];     // 04-05
        uint8_t     volume_id;           // 06
        uint8_t     _not_used_07;        // 07
        uint8_t     volume_name[31];     // 08-26
        uint8_t     pairs_on_sector;     // 27
        uint8_t     _not_used_28[8];     // 28-2F
        uint8_t     last_track;          // 30
        uint8_t     direction;           // 31
        uint8_t     _not_used_32[2];     // 32-33
        uint8_t     tracks_total;        // 34
        uint8_t     sectors_on_track;    // 35
        uint16_t    bytes_per_sector;    // 36-37
        uint32_t    free_sectors[50];    // 38-FF
    };

    struct Agat_VTOC_Ex
    {
        uint32_t    free_sectors[64];
    };

    struct Apple_DOS_File
    {
        uint8_t     tbl_track;      // 00
        uint8_t     tbl_sector;     // 01
        uint8_t     type;           // 02
        uint8_t     name[30];       // 03-20
        uint16_t    size;           // 21-22
    };

    struct Apple_DOS_Catalog
    {
        uint8_t         _not_used_00;       // 00
        uint8_t         next_track;         // 01
        uint8_t         next_sector;        // 02
        uint8_t         _not_used_03[8];    // 03-0A
        Apple_DOS_File  files[7];           // 0B-FF
    };

    struct Apple_DOS_TS_List
    {
        uint8_t         _not_used_00;       // 00
        uint8_t         next_track;         // 01
        uint8_t         next_sector;        // 02
        uint8_t         _not_used_03[2];    // 03-04
        uint16_t        offset;             // 05-06
        uint8_t         _not_used_07[5];    // 07-0B
        uint8_t         ts[122][2];         // 0C-FF
    };

    struct FIL_header
    {
        uint8_t     name[30];
        uint8_t     tsl[9];
        uint8_t     type;
    };

    struct TS_PAIR
    {
        uint8_t     track;
        uint8_t     sector;
    };

    constexpr uint32_t VTOCMask140[32] = {
        0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, // 0..7
        0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, // 8..F
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    };

    constexpr uint32_t VTOCMask840[32] = {
         0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000, 0x00000100, 0x00000200, 0x00000400, // 00..07
         0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00000001, 0x00000002, 0x00000004, // 08..0F
         0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000000, 0x00000000, 0x00000000, // 10..14
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    };

    #pragma pack(pop)


    class fsDOS33: public fileSystem
    {
    protected:
        Agat_VTOC * VTOC;
        TS_PAIR current_dir;
        std::vector<TS_PAIR> current_path;
        int attr_to_type(uint8_t a);
        bool find_epmty_dir_entry(Apple_DOS_File *& dir_entry, bool just_check, bool &extra_sector);
        bool find_empty_sector(uint8_t start_track, TS_PAIR & ts, bool go_forward);
        bool sector_is_free(int head, int track, int sector) override;
        void sector_free(int head, int track, int sector) override;
        bool sector_occupy(int head, int track, int sector) override;
        int free_sectors() override;
        virtual uint32_t * track_map(int track);

    public:
        explicit fsDOS33(diskImage * image);
        int open() override;
        FSCaps getCaps() override;
        FS getFS() const override {return FS::DOS33;};
        void cd(const dsk_tools::fileData & dir) override;
        void cd_up() override;
        int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) override;
        BYTES get_file(const fileData & fd) override;
        Result get_file(const UniversalFile & uf, BYTES & data) const override;
        Result put_file(const UniversalFile & uf, const BYTES & data, bool force_replace = false) override;
        Result delete_file(const UniversalFile & uf) override;
        std::string file_info(const fileData & fd) override;
        int file_delete(const fileData & fd) override;
        int file_add(const std::string & file_name, const std::string & format_id) override;
        std::vector<std::string> get_save_file_formats() override;
        std::vector<std::string> get_add_file_formats() override;
        int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
        std::string information() override;
        int mkdir(const std::string & dir_name) override;
        int file_rename(const fileData & fd, const std::string & new_name) override;
        bool is_root() override;
        std::vector<ParameterDescription> file_get_metadata(const fileData & fd) override;
        int file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata) override;
        bool file_find(const std::string & file_name, fileData &fd) override;
    };
}

#endif // FS_DOS33_H
