// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the ProDOS filesystem (Agat Nippel OS, Apple II)

#pragma once

#include "filesystem.h"

namespace dsk_tools {

    constexpr unsigned PRODOS_BLOCK_SIZE      = 512;
    constexpr unsigned PRODOS_ROOT_BLOCK      = 2;      // The volume directory always starts here
    constexpr unsigned PRODOS_ENTRY_LENGTH    = 39;
    constexpr unsigned PRODOS_ENTRIES_PER_BLK = 13;
    constexpr unsigned PRODOS_DIR_HEADER_LEN  = 4;      // prev/next block pointers before the first entry
    constexpr unsigned PRODOS_INDEX_ENTRIES   = 256;    // pointers in an index block

    // Storage types, the high nibble of the first byte of a directory entry
    constexpr uint8_t PRODOS_ST_DELETED    = 0x0;
    constexpr uint8_t PRODOS_ST_SEEDLING   = 0x1;       // A single data block
    constexpr uint8_t PRODOS_ST_SAPLING    = 0x2;       // An index block with up to 256 data blocks
    constexpr uint8_t PRODOS_ST_TREE       = 0x3;       // A master index block with up to 256 index blocks
    constexpr uint8_t PRODOS_ST_PASCAL     = 0x4;       // A Pascal area on a ProDOS volume
    constexpr uint8_t PRODOS_ST_EXTENDED   = 0x5;       // Two forks, described by a key block
    constexpr uint8_t PRODOS_ST_SUBDIR     = 0xD;
    constexpr uint8_t PRODOS_ST_SUBDIR_HDR = 0xE;
    constexpr uint8_t PRODOS_ST_VOLUME_HDR = 0xF;

    // Access bits
    constexpr uint8_t PRODOS_ACCESS_READ    = 0x01;
    constexpr uint8_t PRODOS_ACCESS_WRITE   = 0x02;
    constexpr uint8_t PRODOS_ACCESS_BACKUP  = 0x20;
    constexpr uint8_t PRODOS_ACCESS_RENAME  = 0x40;
    constexpr uint8_t PRODOS_ACCESS_DESTROY = 0x80;

    // File types
    constexpr uint8_t PRODOS_TYPE_TEXT      = 0x04;
    constexpr uint8_t PRODOS_TYPE_BINARY    = 0x06;
    constexpr uint8_t PRODOS_TYPE_DIRECTORY = 0x0F;
    constexpr uint8_t PRODOS_TYPE_INTEGER   = 0xFA;
    constexpr uint8_t PRODOS_TYPE_APPLESOFT = 0xFC;

    #pragma pack(push, 1)

    // A directory entry describing a file. All three entry kinds are 39 bytes long
    // and share the first 16 bytes; they differ from offset $10 on.
    struct ProDOS_File_Entry {
        uint8_t  storage_name_length;  // 0x00: storage type (high nibble) + name length (low nibble)
        uint8_t  file_name[15];        // 0x01
        uint8_t  file_type;            // 0x10
        uint16_t key_pointer;          // 0x11: data, index or master index block
        uint16_t blocks_used;          // 0x13
        uint8_t  eof[3];               // 0x15: file size in bytes
        uint16_t creation_date;        // 0x18
        uint16_t creation_time;        // 0x1A
        uint8_t  version;              // 0x1C
        uint8_t  min_version;          // 0x1D
        uint8_t  access;               // 0x1E
        uint16_t aux_type;             // 0x1F: load address for BIN, record length for TXT
        uint16_t last_mod_date;        // 0x21
        uint16_t last_mod_time;        // 0x23
        uint16_t header_pointer;       // 0x25: key block of the directory holding this entry
    };

    // The first entry of the volume directory (block 2)
    struct ProDOS_Volume_Header {
        uint8_t  storage_name_length;  // 0x00
        uint8_t  file_name[15];        // 0x01
        uint8_t  reserved[8];          // 0x10
        uint16_t creation_date;        // 0x18
        uint16_t creation_time;        // 0x1A
        uint8_t  version;              // 0x1C
        uint8_t  min_version;          // 0x1D
        uint8_t  access;               // 0x1E
        uint8_t  entry_length;         // 0x1F: 39 for every known implementation
        uint8_t  entries_per_block;    // 0x20: 13
        uint16_t file_count;           // 0x21
        uint16_t bit_map_pointer;      // 0x23: first block of the free space bitmap
        uint16_t total_blocks;         // 0x25
    };

    // A fork of an extended file. Two of these live in the key block of such a file:
    // the data fork at offset 0 and the resource fork at offset $100.
    struct ProDOS_Fork_Entry {
        uint8_t  storage_type;         // 0x00: seedling, sapling or tree
        uint16_t key_pointer;          // 0x01
        uint16_t blocks_used;          // 0x03
        uint8_t  eof[3];               // 0x05
    };

    constexpr unsigned PRODOS_RESOURCE_FORK_OFFSET = 256;

    // The first entry of a subdirectory
    struct ProDOS_Subdir_Header {
        uint8_t  storage_name_length;  // 0x00
        uint8_t  file_name[15];        // 0x01
        uint8_t  reserved[8];          // 0x10
        uint16_t creation_date;        // 0x18
        uint16_t creation_time;        // 0x1A
        uint8_t  version;              // 0x1C
        uint8_t  min_version;          // 0x1D
        uint8_t  access;               // 0x1E
        uint8_t  entry_length;         // 0x1F
        uint8_t  entries_per_block;    // 0x20
        uint16_t file_count;           // 0x21
        uint16_t parent_pointer;       // 0x23: block of the directory holding this subdirectory
        uint8_t  parent_entry_number;  // 0x25
        uint8_t  parent_entry_length;  // 0x26
    };

    #pragma pack(pop)

    class fsProDOS: public fileSystem
    {
    protected:
        ProDOS_Volume_Header VH{};
        unsigned entry_length = PRODOS_ENTRY_LENGTH;
        unsigned entries_per_block = PRODOS_ENTRIES_PER_BLK;
        unsigned total_blocks = 0;                  // as declared by the volume header
        unsigned disk_blocks = 0;                   // as offered by the image geometry
        unsigned sectors_per_block = 1;
        std::vector<uint16_t> current_path;         // key blocks of directories, [0] is the volume one

        Result read_block(unsigned block, BYTES & out) const;
        // Reads a whole directory chain into a plain sequence of entries
        Result read_directory(unsigned key_block, std::vector<BYTES> & entries, std::vector<unsigned> & entry_blocks) const;
        // Where the data of a file (or of one fork of an extended file) starts
        struct Fork {
            uint8_t  storage_type = 0;
            unsigned key_pointer = 0;
            uint32_t eof = 0;
            bool     present = false;
        };
        Result read_fork(const ProDOS_File_Entry & entry, bool resource, Fork & fork) const;
        Result file_blocks(const Fork & fork, std::vector<unsigned> & blocks) const;
        bool block_to_hts(unsigned block, unsigned index, unsigned & head, unsigned & track, unsigned & sector) const;
        unsigned free_blocks() const;
        static std::string entry_name(const uint8_t * entry);
        static std::string type_label(uint8_t file_type);
        static std::string date_time(uint16_t date, uint16_t time);

    public:
        explicit fsProDOS(diskImage * image);
        Result open() override;
        FSCaps get_caps() override;
        FS get_fs() const override {return FS::ProDOS;};
        std::string get_delimiter() override;
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

        // Recognizes a ProDOS volume by its directory header, used by the format detection
        static bool volume_header_is_valid(const BYTES & block, unsigned disk_blocks);
    };
}
