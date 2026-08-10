// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the Iskra-226

#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_iskra226.h"

#define FROM_BE_16(a) (a[0] << 8 | a[1])

namespace dsk_tools {

    fsIskra226::fsIskra226(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsIskra226::get_caps()
    {
        return FSCaps::Types;
    }

    Result fsIskra226::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        const uint8_t * s0 = image->get_sector_data(0, 0, 0);
        m_list_size =  s0[0] << 8 | s0[1]; //big-endian
        m_files_end =  s0[2] << 8 | s0[3];
        m_size_total = s0[4] << 8 | s0[5];

        if (m_list_size < 50 && m_list_size > 0 && m_size_total <= 1000 && m_files_end <= m_size_total)
        {
            m_charmap = init_charmap("koi8_r");
            is_open = true;
            return Result::ok();
        }

        return Result::error(ErrorCode::OpenBadFormat);
    }

    std::vector<std::string> fsIskra226::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    Result fsIskra226::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const auto SPT = image->get_sectors();
        constexpr unsigned dir_enries = 128 / sizeof(ISKRA226_DIR_ENTRY);
        // Each logical sector (256 bytes) contains 2 physical sectors (128 bytes)
        for (int ls = 0; ls < m_list_size*2; ls++) {
            const unsigned track = ls / SPT;
            const unsigned sector = ls % SPT;
            auto * dir = reinterpret_cast<ISKRA226_DIR_ENTRY*>(image->get_sector_data(0, track, sector));
            for (unsigned i = 0; i < dir_enries; i++)
                if (dir[i].T[0] != 0) {
                    std::string file_name;
                    for (const unsigned char j : dir[i].NM) file_name += (*m_charmap.charmap)[j];

                    unsigned T = FROM_BE_16(dir[i].T);

                    UniversalFile f;
                    f.is_dir = false;
                    f.is_deleted = (T==0x1180) || (T==0x1100);
                    f.name = trim(file_name);
                    f.size = (FROM_BE_16(dir[i].LA) - FROM_BE_16(dir[i].FI)) * 256;

                    if (T==0x1080 || T==0x1180) f.type_label = "ПФ";
                    if (T==0x1000 || T==0x1100) f.type_label = "ФД";

                    f.metadata.resize(sizeof(ISKRA226_DIR_ENTRY));
                    std::memcpy(f.metadata.data(), &dir[i], sizeof(ISKRA226_DIR_ENTRY));

                    files.push_back(f);
                }
        }


        return Result::ok();
    }

    Result fsIskra226::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        data.clear();
        const auto dir_entry = reinterpret_cast<const ISKRA226_DIR_ENTRY *>(uf.metadata.data());
        const unsigned first_sector = FROM_BE_16(dir_entry->FI);
        const unsigned last_sector = FROM_BE_16(dir_entry->LA);
        const auto SPT = image->get_sectors();
        const auto sector_size = image->get_sector_size();

        if (first_sector<=last_sector && last_sector<=m_size_total) {
            for (unsigned ls = first_sector*2; ls <= last_sector*2+1; ls++) {
                const unsigned track = ls / SPT;
                const unsigned sector = ls % SPT;
                auto * sector_data = image->get_sector_data(0, track, sector);
                data.insert(data.end(), sector_data, sector_data + sector_size);
            }

            return Result::ok();
        }
        return Result::error(ErrorCode::ReadError);
    }

}