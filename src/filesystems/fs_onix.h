// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the Onix (ONYX) OS filesystem, an Acorn MOS port to Agat

#pragma once

#include "filesystem.h"

namespace dsk_tools {

    // An Onix volume is an 840 Kb Agat disk seen as a flat run of 256 byte blocks,
    // block number = track * sectors_per_track + sector
    constexpr unsigned ONIX_BLOCK_SIZE      = 256;
    constexpr unsigned ONIX_BOOT_BLOCK      = 0;
    constexpr unsigned ONIX_FAT_BLOCK       = 1;        // the allocation table lives in blocks 1..20
    constexpr unsigned ONIX_FAT_BLOCKS      = 20;
    constexpr unsigned ONIX_FAT_ENTRIES     = ONIX_FAT_BLOCKS * ONIX_BLOCK_SIZE / 2;
    constexpr unsigned ONIX_ENTRY_LENGTH    = 21;
    constexpr unsigned ONIX_ENTRIES_PER_BLK = 12;       // 12 * 21 = 252, the last 4 bytes are unused
    constexpr unsigned ONIX_NAME_LENGTH     = 10;

    // Allocation table values
    constexpr uint16_t ONIX_BLOCK_FREE      = 0x0000;   // never allocated, or freed again
    constexpr uint16_t ONIX_CHAIN_END       = 0xFFFE;
    constexpr uint16_t ONIX_BLOCK_SYSTEM    = 0xFFFF;   // the area the OS image itself occupies

    // The first byte of a directory entry
    constexpr uint8_t  ONIX_NAME_END        = 0x00;     // no more entries in this block
    constexpr uint8_t  ONIX_NAME_DELETED    = 0xFF;

    // The two upper bits of the attribute byte tell the three kinds of entry apart.
    // The OS itself branches on exactly this: LDA attr / AND #$C0 / CMP #$80
    constexpr uint8_t  ONIX_TYPE_MASK       = 0xC0;
    constexpr uint8_t  ONIX_TYPE_FILE       = 0x40;     // word_a = load, word_b = length, word_c = exec
    constexpr uint8_t  ONIX_TYPE_SEQ        = 0x80;     // a *SPOOL / OPENOUT file: word_a = length
    constexpr uint8_t  ONIX_TYPE_DIR        = 0xC0;     // start_block is the first block of a directory

    #pragma pack(push, 1)

    struct Onix_Dir_Entry {
        uint8_t  name[ONIX_NAME_LENGTH];   // 0x00: padded with spaces, case is significant
        uint8_t  month;                    // 0x0A: month in the low nibble, 0 = no date
        uint8_t  day;                      // 0x0B: day in the low 5 bits
        uint16_t start_block;              // 0x0C
        uint16_t word_a;                   // 0x0E
        uint16_t word_b;                   // 0x10
        uint16_t word_c;                   // 0x12
        uint8_t  attributes;               // 0x14
    };

    #pragma pack(pop)

    static_assert(sizeof(Onix_Dir_Entry) == ONIX_ENTRY_LENGTH, "Onix_Dir_Entry must be 21 bytes");

    class fsOnix: public fileSystem
    {
    protected:
        std::vector<uint16_t> m_fat;                    // the allocation table, read once on open()
        unsigned disk_blocks = 0;                       // as offered by the image geometry
        unsigned root_block = 0;                        // m_fat[0]
        std::vector<unsigned> current_path;             // first blocks of directories, [0] is the root

        Result read_block(unsigned block, BYTES & out) const;
        bool block_to_hts(unsigned block, unsigned & head, unsigned & track, unsigned & sector) const;
        // Follows a chain through the allocation table, stopping on the end marker, on a
        // block outside the volume and on a loop
        void block_chain(unsigned start, std::vector<unsigned> & blocks) const;
        // entry_blocks and entry_slots say where each entry was found: its block and its
        // index inside that block, which is not derivable from the entry order because a
        // block may hold fewer than ONIX_ENTRIES_PER_BLK entries
        Result read_directory(unsigned first_block, std::vector<BYTES> & entries,
                              std::vector<unsigned> & entry_blocks, std::vector<unsigned> & entry_slots) const;

        static uint8_t entry_type(const Onix_Dir_Entry & entry) {return entry.attributes & ONIX_TYPE_MASK;}
        static uint32_t entry_length(const Onix_Dir_Entry & entry);
        static std::string entry_name(const Onix_Dir_Entry & entry);
        static std::string type_label(const Onix_Dir_Entry & entry);
        static std::string entry_date(const Onix_Dir_Entry & entry);
        unsigned system_blocks() const;
        unsigned free_blocks() const;

    public:
        explicit fsOnix(diskImage * image);
        Result open() override;
        FSCaps get_caps() override;
        FS get_fs() const override {return FS::Onix;};
        std::string get_delimiter() override;
        std::string get_charmap() const override;
        void update_stats() override;
        void cd(const UniversalFile & dir, bool & updir) override;
        void cd_up() override;
        bool is_root() override;
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        Result find_file(const std::string & file_name, UniversalFile & fd) override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        std::string file_info(const UniversalFile & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        std::string information() override;
        SectorTypeMap get_sector_type_map() override;

        // Recognize an Onix volume, used by the format detection. The allocation table alone
        // is a strong enough signature; the root directory block is checked when available.
        // fat_area is the 20 blocks that follow the boot sector.
        static bool fat_is_valid(const BYTES & fat_area, unsigned total_blocks, unsigned & root_block);
        static bool dir_block_is_valid(const BYTES & block);
        static bool volume_is_valid(const BYTES & image, unsigned total_blocks);
    };
}
