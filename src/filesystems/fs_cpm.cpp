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

    FSCaps fsCPM::get_caps()
    {
        return FSCaps::Protect | FSCaps::Types | FSCaps::Export;
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
        if (image->get_type_id() == "TYPE_PC_360_I" || image->get_type_id() == "TYPE_PC_360_NI") {
            // n = 11, BLS = 2048 (2**n)
            // SPT, BSH, BLM, EXM, DSM, DRM, AL0, AL1, CKS, OFF
            DPB = {
                36,          // SPT: 512*9/128
                4,           // BSH: n-7
                15,          // BLM: 2**BSH - 1
                0,           // EXM: 2**(BHS-2) - 1 if DSM<256
                179,         // DSM: Size/BLS - 1
                63,          // DRM
                0b10000000,  // AL0
                0b00000000,  // AL1
                16,          // CKS
                0            // OFF
            };
        } else
        if (image->get_type_id() == "TYPE_GMD_7012_I") {
            // n = 10, BLS = 1024 (2**n)
            // SPT, BSH, BLM, EXM, DSM, DRM, AL0, AL1, CKS, OFF
            DPB = {
                26,          // SPT: 128*26/128
                3,           // BSH: n-7
                7,           // BLM: 2**BSH - 1
                0,           // EXM: 2**(BHS-2) - 1 if DSM<256
                242,         // DSM: (77-2)*26*128/1024 - 1
                63,          // DRM
                0b11000000,  // AL0
                0b00000000,  // AL1
                16,          // CKS
                2            // OFF
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

    std::string fsCPM::information()
    {
        std::string result;

        result += "{$DPB_INFO}:\n";
        result += "    {$CPM_SECTORS_PER_TRACK}: " + std::to_string(DPB.SPT) + "\n";
        result += "    {$CPM_BLOCK_SIZE}: " + std::to_string(1 << (DPB.BSH + 7)) + "\n";
        result += "    {$CPM_TOTAL_BLOCKS}: " + std::to_string(DPB.DSM + 1) + "\n";
        result += "    {$CPM_DIR_ENTRIES}: " + std::to_string(DPB.DRM + 1) + "\n";
        result += "    {$CPM_RESERVED_TRACKS}: " + std::to_string(DPB.OFF) + "\n";
        result += "\n";

        if (!image->has_bad_sectors()) return result;

        result += "{$DISK_HAS_BAD_SECTORS}\n\n";

        if (DPB.OFF > 0) {
            const int heads = image->get_heads();
            const int sectors = image->get_sectors();
            bool found = false;

            for (unsigned track = 0; track < DPB.OFF; track++) {
                for (int head = 0; head < heads; head++) {
                    for (int s = 1; s <= sectors; s++) {
                        if (image->is_bad_sector(head, track, s)) {
                            result += "{$BAD_SECTOR_IN_RESERVED}: "
                                    + std::to_string(head) + ":"
                                    + std::to_string(track) + ":"
                                    + std::to_string(s) + "\n";
                            found = true;
                        }
                    }
                }
            }

            if (!found) {
                result += "{$NO_BAD_SECTORS_IN_RESERVED}\n";
            }
        }

        {
            const int entries_in_sector = image->get_sector_size() / sizeof(CPM_DIR_ENTRY);
            const int directory_sectors = (DPB.DRM + 1) / entries_in_sector;
            bool found = false;

            for (int i = 0; i < directory_sectors; i++) {
                int s = translate_sector(i);
                if (image->is_bad_sector(0, DPB.OFF, s)) {
                    result += "{$BAD_SECTOR_IN_DIRECTORY}: "
                            + std::to_string(0) + ":"
                            + std::to_string(DPB.OFF) + ":"
                            + std::to_string(s) + "\n";
                    found = true;
                }
            }

            if (!found) {
                result += "{$NO_BAD_SECTORS_IN_DIRECTORY}\n";
            }
        }

        {
            std::vector<UniversalFile> files;
            dir(files, false);

            const int sectors = image->get_sectors();
            const int BLS = 1 << (DPB.BSH + 7);
            const int spb = BLS / image->get_sector_size();
            const int index_shift = DPB.OFF * sectors;
            const int heads = image->get_heads();

            result += "\n";
            bool any_bad = false;

            const int sector_size = image->get_sector_size();

            for (const auto & f : files) {
                std::string sector_map;
                std::string bad_list;
                bool file_bad = false;
                unsigned file_offset = 0;
                for (size_t i = 0; i < f.metadata.size() / sizeof(CPM_DIR_ENTRY); i++) {
                    const auto de = reinterpret_cast<const CPM_DIR_ENTRY *>(f.metadata.data() + i * sizeof(CPM_DIR_ENTRY));
                    for (const unsigned char AL : de->AL) {
                        if (AL != 0 && AL != 0xE5) {
                            for (int k = 0; k < spb; k++) {
                                const int sector_index = AL * spb + k + index_shift;
                                int head, track;
                                if (heads == 1) {
                                    head = 0;
                                    track = sector_index / sectors;
                                } else {
                                    head = (sector_index / sectors) & 1;
                                    track = (sector_index / sectors) >> 1;
                                }
                                const int sector = translate_sector(sector_index % sectors);
                                if (image->is_bad_sector(head, track, sector)) {
                                    sector_map += "B";
                                    bad_list += "    $" + int_to_hex(file_offset, false)
                                              + " - " + std::to_string(head)
                                              + ":" + std::to_string(track)
                                              + ":" + std::to_string(sector) + "\n";
                                    file_bad = true;
                                } else {
                                    sector_map += ".";
                                }
                                file_offset += sector_size;
                            }
                        }
                    }
                }
                if (file_bad) {
                    result += "{$FILE_HAS_BAD_SECTORS}: " + f.name + "\n    " + sector_map + "\n" + bad_list;
                    any_bad = true;
                }
            }

            if (!any_bad) {
                result += "{$NO_FILES_WITH_BAD_SECTORS}\n";
            }
        }

        return result;
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

        const int sector_size = image->get_sector_size();
        const int BLS = 1 << (DPB.BSH + 7);                 // Block size
        const int spb = BLS / sector_size;     // Sectors per block

        const int heads = image->get_heads();
        const int sectors = image->get_sectors();
        const int index_shift = DPB.OFF * sectors;

        int file_size = 0;
        std::string list;
        for (int i=0; i < fd.metadata.size() / sizeof(CPM_DIR_ENTRY); i++) {
            list += "{$EXTENT}: " +  std::to_string(i) + "\n";
            std::memcpy(&dir_entry, fd.metadata.data() + i*sizeof(CPM_DIR_ENTRY), sizeof(CPM_DIR_ENTRY));
            file_size += dir_entry.RC*128;

            for (const unsigned char AL : dir_entry.AL) {
                if (AL != 0 && AL != 0xE5)  {
                    list += "    {$CPM_BLOCK}: " + std::to_string(AL);
                    list += ", {$CPM_SECTORS} ";
                    list += heads==1 ? "(T:S)" : "(H:T:S)";
                    list += ": ";
                    for (int k=0; k<spb; k++) {
                        if (k) list += ", ";
                        const int sector_index = AL*spb + k + index_shift;
                        const int sector = translate_sector(sector_index % sectors);
                        if (heads == 1) {
                            const int track = sector_index / sectors;
                            list += std::to_string(track) + ":" + std::to_string(sector);
                        } else {
                            const int head = (sector_index / sectors) & 1;
                            const int track = (sector_index / sectors) >> 1;
                            list += std::to_string(head) + ":" + std::to_string(track) + ":" + std::to_string(sector);
                        }
                    }
                    list += "\n";
                }
            }
        }
        result += "{$SIZE}: " +  std::to_string(file_size) + " {$BYTES}\n";
        result += "{$ATTRIBUTES}: " + attrs + " \n";
        result += "\n";
        result += "{$CPM_SECTOR_SIZE}: " +  std::to_string(sector_size) + "\n";
        result += "{$CPM_BLOCK_SIZE}: " +  std::to_string(BLS) + "\n";
        result += "{$CPM_SECTORS_PER_BLOCK}: " +  std::to_string(spb) + "\n";
        result += "{$CPM_RESERVED_TRACKS}: " +  std::to_string(DPB.OFF) + "\n";
        result += "\n";
        result += list;

        return result;
    }

    void fsCPM::load_file(const BYTES & dir_records, BYTES & out) const
    {
        out.clear();
        int file_size = 0;
        for (int i=0; i<dir_records.size() / sizeof(CPM_DIR_ENTRY); i++) {
            const auto dir_entry = reinterpret_cast<const CPM_DIR_ENTRY *>(dir_records.data() + i*sizeof(CPM_DIR_ENTRY));

            file_size += dir_entry->RC*128;

            const int sector_size = image->get_sector_size();
            const int BLS = 1 << (DPB.BSH + 7);                 // Block size
            const int spb = BLS / sector_size;     // Sectors per block

            const int sectors = image->get_sectors();
            const int index_shift = DPB.OFF * sectors;

            for (const unsigned char AL : dir_entry->AL) {
                if (AL != 0 && AL != 0xE5)  {
                    for (int k=0; k<spb; k++) {
                        const int sector_index = AL*spb + k + index_shift;
                        int head, track;
                        if (image->get_heads() == 1) {
                            head = 0;
                            track = sector_index / sectors;
                        } else {
                            head = (sector_index / sectors) & 1;
                            track = (sector_index / sectors) >> 1;
                        }
                        const int sector = translate_sector(sector_index % sectors);
                        const auto p = image->get_sector_data(head, track, sector);
                        out.insert(out.end(), p, p + sector_size);
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
        load_file(uf.metadata, data);
        return Result::ok();
    }

    Result fsCPM::dir(std::vector<dsk_tools::UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const int catalog_size = DPB.DRM + 1;
        const int entries_in_sector = image->get_sector_size() / sizeof(CPM_DIR_ENTRY);
        const int directory_sectors = catalog_size / entries_in_sector;

        std::vector<CPM_DIR_ENTRY*> catalog(catalog_size);

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            for (int j = 0; j < entries_in_sector; j++)
                catalog[i*entries_in_sector + j] = reinterpret_cast<CPM_DIR_ENTRY*>(sector + j*sizeof(CPM_DIR_ENTRY));
        }

        std::string prev_name;

        for (int i = 0; i < catalog_size; i++) {
            const uint8_t ST = catalog[i]->ST;
            if (ST == 0xE5) break;
            if (ST != 0x1F) {
                std::string file_name = make_file_name(*catalog[i]);
                const int extent = catalog[i]->XH*32 + catalog[i]->XL;
                if (extent == 0 || prev_name != file_name ) {
                    prev_name = file_name;

                    UniversalFile f;
                    f.is_dir = false;
                    f.is_deleted = false;
                    f.name = file_name;
                    f.size = catalog[i]->RC * 128;

                    f.is_protected = (catalog[i]->E[0] & 0x80) != 0;
                    f.type_label = "";
                    f.type_label += (catalog[i]->E[1] & 0x80)?"S":""; // System (hidden)
                    f.type_label += (catalog[i]->E[2] & 0x80)?"A":""; // Archived


                    std::set<std::string> txts = {".txt", ".doc", ".pas", ".asm", ".cmd", ".hlp"};
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
                    size_t old_size = f->metadata.size();
                    f->metadata.resize(old_size + sizeof(CPM_DIR_ENTRY));
                    std::memcpy(f->metadata.data() + old_size, catalog[i], sizeof(CPM_DIR_ENTRY));
                    std::cout << f->metadata.size() << std::endl;
                }
            }
        }

        return Result::ok();
    }

}
