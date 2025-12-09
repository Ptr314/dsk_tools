// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the CP/M filesystem

#include <iostream>
#include <fstream>
#include <cstring>
#include <set>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_cpm.h"

namespace dsk_tools {

fsCPM::fsCPM(diskImage * image, const std::string &filesystem_id):
        fileSystem(image)
        , m_filesystem_id(filesystem_id)
    {}

    FSCaps fsCPM::getCaps()
    {
        return FSCaps::Protect | FSCaps::Types;
    }

    Result fsCPM::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        if (image->get_type_id() == "TYPE_AGAT_140") {
            // n = 10, BLS = 1024 (2**n)
            // SPT, BSH, BLM, EXM, DSM, DRM, AL0, AL1, CKS, OFF
            DPB = {
                32,          // SPT: 256*16/128
                3,           // BSH: n-7
                7,           // BLM: 2**BSH - 1
                0,           // EXM: 2**(BHS-2) - 1 if DSM<256
                127,         // DSM
                63,          // DRM
                0b11000000,  // AL0
                0b00000000,  // AL1
                16,          // CKS
                3            // OFF
            };
        } else
            return Result::error(ErrorCode::OpenBadFormat, "Unsupported disk type for CP/M");

        is_open = true;
        return Result::ok();
    }

    std::string fsCPM::make_file_name(CPM_DIR_ENTRY & di)
    {
        std::string ext;
        for (const unsigned char i : di.E) ext += static_cast<char>(i & 0x7F);
        return trim(std::string(reinterpret_cast<char*>(&di.F), 8)) + ((!ext.empty())?("."+ext):"");
    }

    int fsCPM::translate_sector(int sector) const
    {
        if (m_filesystem_id == "FILESYSTEM_CPM_RAW")
            return sector;
        else
        if (m_filesystem_id == "FILESYSTEM_CPM_DOS")
            return agat_140_cpm2dos[sector];
        else
        if (m_filesystem_id == "FILESYSTEM_CPM_PRODOS")
            return agat_140_cpm2prodos[sector];
        else
            throw std::runtime_error("Incorrect filesystem id");
    }

    std::string fsCPM::file_info(const UniversalFile & fd) {

        std::string result;
        std::string attrs;

        CPM_DIR_ENTRY dir_entry{};
        std::memcpy(&dir_entry, fd.metadata.data(), sizeof(CPM_DIR_ENTRY));

        attrs += (dir_entry.E[0] & 0x80)?"P":"-"; // Read-only
        attrs += (dir_entry.E[1] & 0x80)?"S":"-"; // System (hidden)
        attrs += (dir_entry.E[2] & 0x80)?"A":"-"; // Archived

        result += "{$FILE_NAME}: " +  make_file_name(dir_entry) + "\n";

        int file_size = 0;
        std::string list;
        for (int i=0; i < fd.metadata.size() / sizeof(CPM_DIR_ENTRY); i++) {
            list += "{$EXTENT}: " +  std::to_string(i) + "\n";
            std::memcpy(&dir_entry, fd.metadata.data() + i*sizeof(CPM_DIR_ENTRY), sizeof(CPM_DIR_ENTRY));
            file_size += dir_entry.RC*128;
            for (const unsigned char AL : dir_entry.AL) {
                const int track = AL / 4 + DPB.OFF;
                const int part = AL % 4;
                if (AL != 0 && AL != 0xE5)  {
                    list += "    Block: " + std::to_string(AL);
                    list += " Track: " + std::to_string(track);
                    list += " Part: " + std::to_string(part);
                    list += " Sectors:" ;
                    for (int k=0; k<4; k++) {
                        list += " " + std::to_string(translate_sector(part*4 + k));
                    }
                    list += "\n";
                }
            }
        }

        result += "{$SIZE}: " +  std::to_string(file_size) + " {$BYTES}\n";
        result += "{$ATTRIBUTES}: " + attrs + " \n";

        result += "\n";
        result += list;

        return result;
    }

    void fsCPM::load_file(const BYTES * dir_records, int extents, BYTES & out) const
    {
        out.clear();
        int file_size = 0;
        for (int i=0; i<extents; i++) {
            CPM_DIR_ENTRY dir_entry{};
            std::memcpy(&dir_entry, dir_records + i*sizeof(CPM_DIR_ENTRY), sizeof(CPM_DIR_ENTRY));

            file_size += dir_entry.RC*128;

            for (const unsigned char AL : dir_entry.AL) {
                const int track = AL / 4 + DPB.OFF;
                const int part = AL % 4;
                if (AL != 0 && AL != 0xE5)  {
                    for (int k=0; k<4; k++) {
                        const int sector = translate_sector(part*4 + k);
                        const auto p = image->get_sector_data(0, track, sector);
                        out.insert(out.end(), p, p + 256);
                    }
                }
            }
        }
        out.resize(file_size);
    }

    std::vector<std::string> fsCPM::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    std::vector<std::string> fsCPM::get_add_file_formats()
    {
        return {"FILE_BINARY"};
    }

    Result fsCPM::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        data.clear();
        load_file(reinterpret_cast<const BYTES*>(uf.metadata.data()), uf.metadata.size() / sizeof(CPM_DIR_ENTRY), data);
        return Result::ok();
    }

    Result fsCPM::dir(std::vector<dsk_tools::UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        constexpr int directory_sectors = 6;
        constexpr int entries_in_sector = 8;
        constexpr int catalog_size = directory_sectors * entries_in_sector;

        std::vector<CPM_DIR_ENTRY*> catalog(catalog_size);

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            for (int j = 0; j < entries_in_sector; j++)
                catalog[i*entries_in_sector + j] = reinterpret_cast<CPM_DIR_ENTRY*>(sector + j*sizeof(CPM_DIR_ENTRY));
        }

        for (int i = 0; i < catalog_size; i++) {
            const uint8_t ST = catalog[i]->ST;
            if (ST != 0xE5 && ST != 0x1F) {
                const int extent = catalog[i]->XH*32 + catalog[i]->XL;
                if (extent == 0) {
                    UniversalFile f;
                    f.is_dir = false;
                    f.is_deleted = false;
                    f.name = make_file_name(*catalog[i]);
                    f.size = catalog[i]->RC * 128;

                    f.is_protected = (catalog[i]->E[0] & 0x80) != 0;
                    f.type_label = "";
                    f.type_label += (catalog[i]->E[1] & 0x80)?"S":""; // System (hidden)
                    f.type_label += (catalog[i]->E[2] & 0x80)?"A":""; // Archived


                    std::set<std::string> txts = {".txt", ".doc", ".pas", ".asm"};
                    std::string ext = get_file_ext(f.name);
                    f.type_preferred = (txts.find(ext) != txts.end())?PreferredType::Text:PreferredType::Binary;
                    if (ext == ".bas")
                        f.type_preferred = PreferredType::MBASIC;

                    f.metadata.resize(sizeof(CPM_DIR_ENTRY));
                    std::memcpy(f.metadata.data(), catalog[i], sizeof(CPM_DIR_ENTRY));

                    files.push_back(f);
                } else {
                    UniversalFile * f = &(files.at(files.size()-1));
                    f->size += catalog[i]->RC * 128;
                    f->metadata.resize(f->metadata.size() + sizeof(CPM_DIR_ENTRY));
                    std::memcpy(f->metadata.data() + sizeof(CPM_DIR_ENTRY)*extent, catalog[i], sizeof(CPM_DIR_ENTRY));
                }
            }
        }

        return Result::ok();
    }

}
