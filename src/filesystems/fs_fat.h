// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the PC FAT filesystem

#pragma once

#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    struct FAT_BPB {
        uint8_t  jmpBoot[3];           // 0x00
        uint8_t  OEMName[8];           // 0x03
        uint16_t bytesPerSector;       // 0x0B
        uint8_t  sectorsPerCluster;    // 0x0D
        uint16_t reservedSectors;      // 0x0E
        uint8_t  numFATs;              // 0x10
        uint16_t rootEntries;          // 0x11
        uint16_t totalSectors16;       // 0x13
        uint8_t  mediaDescriptor;      // 0x15
        uint16_t sectorsPerFAT16;      // 0x16
        uint16_t sectorsPerTrack;      // 0x18
        uint16_t numHeads;             // 0x1A
        uint32_t hiddenSectors;        // 0x1C
        uint32_t totalSectors32;       // 0x20
    };

    struct FAT_DIR_ENTRY {
        uint8_t  name[11];             // 0x00: 8.3 name, blank-padded, no dot
        uint8_t  attr;                 // 0x0B
        uint8_t  ntReserved;           // 0x0C
        uint8_t  createTimeTenth;      // 0x0D
        uint16_t createTime;           // 0x0E
        uint16_t createDate;           // 0x10
        uint16_t accessDate;           // 0x12
        uint16_t firstClusterHi;       // 0x14
        uint16_t writeTime;            // 0x16
        uint16_t writeDate;            // 0x18
        uint16_t firstClusterLo;       // 0x1A
        uint32_t fileSize;             // 0x1C
    };

    #pragma pack(pop)

    enum class FATType { FAT12, FAT16, FAT32 };

    constexpr uint8_t FAT_ATTR_READ_ONLY = 0x01;
    constexpr uint8_t FAT_ATTR_HIDDEN    = 0x02;
    constexpr uint8_t FAT_ATTR_SYSTEM    = 0x04;
    constexpr uint8_t FAT_ATTR_VOLUME_ID = 0x08;
    constexpr uint8_t FAT_ATTR_DIRECTORY = 0x10;
    constexpr uint8_t FAT_ATTR_ARCHIVE   = 0x20;
    constexpr uint8_t FAT_ATTR_LONG_NAME = 0x0F; // RO | HID | SYS | VOL

    class fsFAT: public fileSystem
    {
    protected:
        FAT_BPB BPB{};
        FATType fat_type = FATType::FAT12;
        unsigned fat_start = 0;          // LBA of the first FAT
        unsigned root_dir_start = 0;     // LBA of root directory (FAT12/16)
        unsigned root_dir_sectors = 0;   // sectors occupied by root directory (FAT12/16)
        unsigned data_start = 0;         // LBA of the first data cluster (cluster #2)
        unsigned total_clusters = 0;
        std::vector<uint32_t> current_path; // cluster numbers; 0 = root (FAT12/16)

        uint8_t * read_lba(unsigned lba) const;
        unsigned read_fat_entry(unsigned cluster) const;
        bool is_eoc(unsigned fat_entry) const;
        Result read_directory(uint32_t cluster, BYTES & out) const;
        static std::string make_file_name(const FAT_DIR_ENTRY & de);

    public:
        explicit fsFAT(diskImage * image);
        Result open() override;
        FSCaps get_caps() override;
        FS get_fs() const override {return FS::FAT;};
        void cd(const UniversalFile & dir, bool & updir) override;
        void cd_up() override;
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        std::string file_info(const UniversalFile & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        std::vector<std::string> get_add_file_formats() override;
        std::string information() override;
        bool is_root() override;
    };
}
