// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the Iskra-226
#pragma once


#include "charmaps.h"
#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    struct ISKRA226_DIR_ENTRY
    {
        // Numbers are big-endian
        uint8_t     T[2];       // Type: 1080: Code, 1180: Deleted code, 1000: data, 1100: deleted data
        uint8_t     FI[2];      // First sector
        uint8_t     LA[2];      // Last sector
        uint8_t     ZZ[2];      // Zeroes
        uint8_t     NM[8];      // File name, KOI-8, space padded
    };

    #pragma pack(pop)

    class fsIskra226: public fileSystem
    {
    protected:
        unsigned m_list_size = 0;
        unsigned m_files_end = 0;
        unsigned m_size_total = 0;

        CharmapInfo m_charmap = {};

    public:
        fsIskra226(diskImage * image);
        FS get_fs() const override {return FS::Iskra226;};
        Result open() override;
        FSCaps get_caps() override;
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        std::vector<std::string> get_save_file_formats() override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
    };
}
