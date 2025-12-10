// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the CP/M filesystem
#pragma once


#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    // https://stjarnhimlen.se/apple2/Apple.CPM.ref.txt
    struct CPM_DPB
    {
        uint16_t   SPT;     // Total number of sectors per track
        uint8_t    BSH;     // Data allocation block shift factor
        uint8_t    BLM;     // Data allocation block mask
        uint8_t    EXM;     // Extent mask
        uint16_t   DSM;     // Total storage capacity of disk drive in blocks
        uint16_t   DRM;     // Total number of directory entries minus one
        uint8_t    AL0;     // Determines reserved directory blocks
        uint8_t    AL1;     // Determines reserved directory blocks
        uint16_t   CKS;     // Size of directory check vector
        uint16_t   OFF;     // No of reserved tracks at beginning of logical disk

    };

    // https://ciderpress2.com/formatdoc/CPM-notes.html
    struct CPM_DIR_ENTRY
    {
        uint8_t     ST;         // Status
        uint8_t     F[8];       // Name
        uint8_t     E[3];       // Extension
        uint8_t     XL;         // Extent number, low part
        uint8_t     BC;         // Depends on OS
        uint8_t     XH;         // Extent number, high part
        uint8_t     RC;         // Records in the extent
        uint8_t     AL[16];     // Allocation table
    };

    #pragma pack(pop)

    class fsCPM: public fileSystem
    {
    protected:
        CPM_DPB DPB{};
        std::string m_filesystem_id;
        static std::string make_file_name(CPM_DIR_ENTRY & di);
        void load_file(const BYTES *dir_records, int extents, BYTES & out) const;

    public:
        fsCPM(diskImage * image, const std::string & filesystem_id);
        FS get_fs() const override {return FS::CPM;};
        Result open() override;
        FSCaps get_caps() override;
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        std::string file_info(const UniversalFile & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        std::vector<std::string> get_add_file_formats() override;
        int translate_sector(int sector) const override;
    };
}
