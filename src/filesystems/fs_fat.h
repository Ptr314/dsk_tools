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

    // VFAT Long File Name (LFN) entry. Sits in the directory slot that would
    // normally hold a FAT_DIR_ENTRY; attr == 0x0F identifies it.
    struct FAT_LFN_ENTRY {
        uint8_t  ord;                  // 0x00: sequence; 0x40 bit = last (highest-ord) entry
        uint8_t  name1[10];            // 0x01: chars 1..5  (UTF-16LE)
        uint8_t  attr;                 // 0x0B: always 0x0F
        uint8_t  type;                 // 0x0C: 0 for ordinary LFN
        uint8_t  checksum;             // 0x0D: checksum of associated short name
        uint8_t  name2[12];            // 0x0E: chars 6..11 (UTF-16LE)
        uint16_t firstClusterLo;       // 0x1A: always 0
        uint8_t  name3[4];             // 0x1C: chars 12..13 (UTF-16LE)
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

        // Writing
        unsigned cluster_bytes() const;
        unsigned eoc_value() const;
        bool write_fat_entry(unsigned cluster, unsigned value);
        unsigned count_free_clusters() const;
        bool allocate_clusters(unsigned count, std::vector<uint32_t> & out);
        bool link_chain(const std::vector<uint32_t> & chain);
        bool free_chain(uint32_t first);
        bool write_cluster(uint32_t cluster, const uint8_t * data, size_t size);
        std::vector<FAT_DIR_ENTRY *> dir_slots(uint32_t dir_cluster) const;
        FAT_DIR_ENTRY * find_live_entry(uint32_t dir_cluster, const uint8_t name[11], unsigned * index = nullptr) const;
        Result prepare_free_slot(uint32_t dir_cluster, unsigned & index, bool & need_extend) const;
        Result take_free_slot(uint32_t dir_cluster, FAT_DIR_ENTRY *& slot, unsigned & index);
        void mark_lfn_deleted(uint32_t dir_cluster, unsigned index);
        FAT_DIR_ENTRY * locate(const UniversalFile & uf) const;
        UniversalFile make_universal_file(const FAT_DIR_ENTRY & de, uint32_t dir_cluster, unsigned index, const std::string & name) const;
        static Result make_short_name(const UniversalFile & uf, uint8_t out[11]);
        static Result make_short_name(const std::string & name, uint8_t out[11]);
        static void fat_now(uint16_t & date, uint16_t & time);

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
        SectorTypeMap get_sector_type_map() override;
        void update_stats() override;

        Result put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace) override;
        Result delete_file(const UniversalFile & uf) override;
        Result restore_file(const UniversalFile & uf) override;
        Result rename_file(const UniversalFile & fd, const std::string & new_name) override;
        Result mkdir(const std::string & dir_name, UniversalFile & new_dir) override;
        Result mkdir(const UniversalFile & uf, UniversalFile & new_dir) override;
        std::vector<ParameterDescription> file_get_metadata(const UniversalFile & fd) override;
        Result file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata) override;
    };
}
