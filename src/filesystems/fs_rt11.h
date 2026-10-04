// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the DEC RT-11 filesystem (BK, DVK, UKNC)

#pragma once

#include "filesystem.h"

namespace dsk_tools {

    // An RT-11 volume is a flat run of 512 byte blocks. Blocks 0..5 hold the boot block,
    // the home block (1) and the secondary bootstrap, the directory follows as a list of
    // two block segments and every file is one contiguous run of blocks.
    constexpr unsigned RT11_BLOCK_SIZE      = 512;
    constexpr unsigned RT11_HOME_BLOCK      = 1;
    constexpr unsigned RT11_DEFAULT_DIR     = 6;
    constexpr unsigned RT11_SEGMENT_BLOCKS  = 2;
    constexpr unsigned RT11_SEGMENT_SIZE    = RT11_SEGMENT_BLOCKS * RT11_BLOCK_SIZE;
    constexpr unsigned RT11_SEGMENT_HEADER  = 10;       // 5 words
    constexpr unsigned RT11_ENTRY_LENGTH    = 14;       // 7 words, plus the extra bytes of the volume
    constexpr unsigned RT11_MAX_SEGMENTS    = 31;

    // Home block fields, byte offsets (octal in the DEC documentation)
    constexpr unsigned RT11_HB_CLUSTER      = 0722;
    constexpr unsigned RT11_HB_DIR_BLOCK    = 0724;
    constexpr unsigned RT11_HB_VERSION      = 0726;
    constexpr unsigned RT11_HB_VOLUME_ID    = 0730;
    constexpr unsigned RT11_HB_OWNER        = 0744;
    constexpr unsigned RT11_HB_SYSTEM_ID    = 0760;
    constexpr unsigned RT11_HB_TEXT_LENGTH  = 12;

    // Status word of a directory entry
    constexpr uint16_t RT11_E_PRE           = 0000020;  // prefix block present
    constexpr uint16_t RT11_E_TENT          = 0000400;  // tentative file
    constexpr uint16_t RT11_E_MPTY          = 0001000;  // empty area
    constexpr uint16_t RT11_E_PERM          = 0002000;  // permanent file
    constexpr uint16_t RT11_E_EOS           = 0004000;  // end of segment marker
    constexpr uint16_t RT11_E_READ          = 0040000;  // read only
    constexpr uint16_t RT11_E_PROT          = 0100000;  // protected from deletion

    // DX (RX01) disks are 77 tracks of 26 sectors of 128 bytes. RT-11 leaves track 0
    // alone and interleaves the rest itself, with a skew that changes from track to track.
    constexpr unsigned RT11_DX_TRACKS       = 77;
    constexpr unsigned RT11_DX_SECTORS      = 26;
    constexpr unsigned RT11_DX_SECTOR_SIZE  = 128;
    constexpr unsigned RT11_DX_BLOCKS       = (RT11_DX_TRACKS - 1) * RT11_DX_SECTORS * RT11_DX_SECTOR_SIZE / RT11_BLOCK_SIZE;

    struct RT11_Entry {
        uint16_t status = 0;
        uint16_t name[3] = {0, 0, 0};               // RAD50: 6 characters of the name, 3 of the type
        uint16_t length = 0;                        // in blocks
        uint16_t job = 0;                           // job and channel of a tentative file
        uint16_t date = 0;
        BYTES    extra;                             // extra bytes per entry, as the volume declares
        unsigned start = 0;                         // the first block, computed while reading
    };

    struct RT11_Segment {
        unsigned number = 0;                        // 1 based
        uint16_t total = 0;                         // segments allocated on the volume
        uint16_t next = 0;                          // 0 for the last one
        uint16_t highest = 0;                       // the highest one in use, kept in segment 1
        uint16_t extra = 0;                         // extra bytes per entry
        uint16_t start = 0;                         // the first block described by this segment
        std::vector<RT11_Entry> entries;
    };

    class fsRT11: public fileSystem
    {
    protected:
        unsigned total_blocks = 0;
        unsigned dir_block = RT11_DEFAULT_DIR;
        BYTES home;

        bool is_dx() const;
        unsigned blocks_inside(const RT11_Entry & entry) const;
        bool block_sectors(unsigned block, std::vector<std::array<unsigned, 3>> & sectors) const;
        Result read_block(unsigned block, BYTES & out) const;
        Result write_block(unsigned block, const uint8_t * data, size_t size);

        Result load_segment(unsigned number, RT11_Segment & segment) const;
        Result save_segment(const RT11_Segment & segment);
        Result load_directory(std::vector<RT11_Segment> & chain) const;
        bool segment_has_room(const RT11_Segment & segment) const;
        Result split_segment(std::vector<RT11_Segment> & chain, size_t index);
        static void consolidate(RT11_Segment & segment);

        bool locate(const std::vector<RT11_Segment> & chain, const UniversalFile & uf, size_t & seg_index, size_t & entry_index) const;
        bool name_exists(const std::vector<RT11_Segment> & chain, const uint16_t name[3], const RT11_Entry * except = nullptr) const;
        UniversalFile make_universal_file(const RT11_Entry & entry, unsigned segment, unsigned index) const;

        static Result make_name(const std::string & file_name, uint16_t out[3]);
        static Result make_name(const UniversalFile & uf, uint16_t out[3]);
        static std::string entry_name(const RT11_Entry & entry);
        static FileDate decode_date(uint16_t date);
        static std::string date_to_string(uint16_t date);
        static bool string_to_date(const std::string & s, uint16_t & date);
        static uint16_t today();
        static std::string home_text(const BYTES & home, unsigned offset);

    public:
        explicit fsRT11(diskImage * image);
        Result open() override;
        FSCaps get_caps() override;
        FS get_fs() const override {return FS::RT11;};
        std::string get_charmap() const override;
        void update_stats() override;
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        Result find_file(const std::string & file_name, UniversalFile & fd) override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        std::string file_info(const UniversalFile & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        std::vector<std::string> get_add_file_formats() override;
        std::string information() override;
        SectorTypeMap get_sector_type_map() override;

        Result put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace) override;
        Result delete_file(const UniversalFile & uf) override;
        Result restore_file(const UniversalFile & uf) override;
        Result rename_file(const UniversalFile & fd, const std::string & new_name) override;
        std::vector<ParameterDescription> file_get_metadata(const UniversalFile & fd) override;
        Result file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata) override;

        // Used by the detection: where an image keeps a DX block, and whether the
        // home block and the first directory segment describe a sane volume
        static void dx_sector(unsigned logical, unsigned & track, unsigned & sector);
        static unsigned directory_block(const BYTES & home, unsigned total_blocks);
        static bool segment_is_valid(const BYTES & segment, unsigned dir_block, unsigned total_blocks);

        static std::string rad50_decode(uint16_t word);
        static bool rad50_encode(const std::string & text, uint16_t & word);
    };
}
