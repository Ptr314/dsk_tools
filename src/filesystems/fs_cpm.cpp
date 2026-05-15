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

fsCPM::fsCPM(diskImage * image, const std::string &filesystem_id, const DiskDefs & diskdefs):
        fileSystem(image)
        , m_filesystem_id(filesystem_id)
        , m_diskdefs(diskdefs)
    {}

    FSCaps fsCPM::get_caps()
    {
        return    FSCaps::Protect | FSCaps::Types | FSCaps::ExAttr | FSCaps::Export
                | FSCaps::Delete  | FSCaps::Add   | FSCaps::Rename | FSCaps::Metadata;
    }

    Result fsCPM::fill_dpb(const std::string & type_id)
    {
        std::string diskdef_id = to_lower(type_id);
        if (type_id.rfind("TYPE_CPM:", 0) == 0)
            diskdef_id = to_lower(type_id.substr(9));

        const auto it = m_diskdefs.find(diskdef_id);
        if (it == m_diskdefs.end())
            return Result::error(ErrorCode::OpenBadFormat, "Unknown CP/M disk definition");

        const DiskDef &diskdef = it->second;

        unsigned heads = 0;
        if (!get_map_value(diskdef.int_params, std::string("heads"), heads, 2, false))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: heads is incorrect");
        unsigned tracks = 0;
        if (!get_map_value(diskdef.int_params, std::string("tracks"), tracks, 0, true))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: tracks is required");
        unsigned sectrk = 0;
        if (!get_map_value(diskdef.int_params, std::string("sectrk"), sectrk, 0, true))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: sectrk is required");
        unsigned seclen = 0;
        if (!get_map_value(diskdef.int_params, std::string("seclen"), seclen, 0, true))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: seclen is required");

        unsigned BLS = 0;
        if (!get_map_value(diskdef.int_params, std::string("blocksize"), BLS, 0, true))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: blocksize is required");
        unsigned n = 0;
        for (unsigned v = BLS; v > 1; v >>= 1) ++n; // Avoiding <cmath> and floating point operations

        unsigned boottrk = 0;
        if (!get_map_value(diskdef.int_params, std::string("boottrk"), boottrk, 0, false))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: boottrk is incorrect");

        const auto OFF = static_cast<uint16_t>(boottrk);

        unsigned maxdir = 0;
        if (!get_map_value(diskdef.int_params, std::string("maxdir"), maxdir, 64, false))
            return Result::error(ErrorCode::OpenBadFormat, "CP/M disk definition: maxdir is incorrect");

        const auto SPT = static_cast<uint16_t>(seclen * sectrk / 128);
        const auto BSH = static_cast<uint8_t>(n - 7);
        const auto BLM = static_cast<uint8_t>((1u << BSH) - 1u);                       // BLS/128 - 1
        const auto DSM = static_cast<uint16_t>((tracks - OFF) * heads * sectrk * seclen / BLS - 1);
        // CP/M 2.2 standard EXM: DSM<256 -> BLS/1024-1, else BLS/2048-1
        const auto  EXM = static_cast<uint8_t>(
            (DSM < 256)
                ? ((BLS >= 1024) ? (BLS / 1024 - 1) : 0)
                : ((BLS >= 2048) ? (BLS / 2048 - 1) : 0)
        );
        const auto DRM = static_cast<uint16_t>(maxdir - 1);
        const auto CKS = static_cast<uint16_t>(maxdir >> 2);                           // removable media: maxdir/4

        // AL0:AL1 reserves the top `dir_blocks` bits of a 16-bit bitmap for the directory.
        // dir_blocks = ceil(maxdir * 32 / BLS)
        const unsigned dir_blocks = (maxdir * 32u + BLS - 1u) / BLS;
        const uint16_t AL_word = (dir_blocks >= 16)
                                     ? static_cast<uint16_t>(0xFFFFu)
                                     : static_cast<uint16_t>(((1u << dir_blocks) - 1u) << (16u - dir_blocks));
        const auto AL0 = static_cast<uint8_t>((AL_word >> 8) & 0xFFu);
        const auto AL1 = static_cast<uint8_t>(AL_word & 0xFFu);

        DPB = {SPT, BSH, BLM, EXM, DSM, DRM, AL0, AL1, CKS, OFF};

        return Result::ok();
    }

    Result fsCPM::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);
        const std::string type_id = image->get_type_id();
        if (type_id == "TYPE_AGAT_140") {
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
        } else {
            if (type_id.rfind("TYPE_CPM:", 0) == 0) {
                const auto res= fill_dpb(type_id);
                if (!res) return res;
            } else
                return Result::error(ErrorCode::OpenBadFormat, "Unsupported disk type for CP/M");
        }

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
                    for (int s = 0; s < sectors; s++) {
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
                                unsigned head, track;
                                if (heads == 1) {
                                    head = 0;
                                    track = sector_index / sectors;
                                } else {
                                    head = (sector_index / sectors) & 1;
                                    track = (sector_index / sectors) >> 1;
                                }
                                unsigned sector = translate_sector(sector_index % sectors);
                                if (image->is_bad_sector(head, track, sector)) {
                                    sector_map += "B";
                                    bad_list += "    $" + int_to_hex(file_offset, true) + " - ";
                                    bad_list += "B:" + std::to_string(AL) + " / ";
                                    bad_list += "L:" + std::to_string(head)
                                              + ":" + std::to_string(track)
                                              + ":" + std::to_string(sector) + " / ";
                                    image->logical_to_physical(head, track, sector);
                                    bad_list += "P:" + std::to_string(head)
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
            const int index_shift = DPB.OFF * sectors * image->get_heads();

            unsigned al_ind = 0;
            while (al_ind < 16) {
                const uint16_t AL = DPB.DSM<256 ? dir_entry->AL[al_ind] : dir_entry->AL[al_ind] + (dir_entry->AL[al_ind+1] << 8);
                al_ind += DPB.DSM<256 ? 1 : 2;
                if (AL != 0 && AL != 0xE5 && AL != 0xE5E5)  {
                    // std::cout << AL << std::endl;
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
        const int entries_in_sector = static_cast<int>(image->get_sector_size() / sizeof(CPM_DIR_ENTRY));
        const int directory_sectors = catalog_size / entries_in_sector;

        std::vector<CPM_DIR_ENTRY*> catalog(catalog_size);

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            for (int j = 0; j < entries_in_sector; j++)
                catalog[i*entries_in_sector + j] = reinterpret_cast<CPM_DIR_ENTRY*>(sector + j*sizeof(CPM_DIR_ENTRY));
        }

        const std::set<std::string> txts = {".txt", ".doc", ".pas", ".asm", ".cmd", ".hlp", ".src"};

        std::string prev_name;
        uint8_t prev_st;

        // Always scan the full directory (DRM+1 entries) — CP/M has no end marker.
        for (int i = 0; i < catalog_size; i++) {
            const uint8_t ST = catalog[i]->ST;
            const bool is_deleted = (ST == 0xE5);

            if (is_deleted) {
                // Slots filled with 0xE5 by format are "never used" — skip them.
                if (catalog[i]->F[0] == 0xE5) continue;
                if (!show_deleted) continue;

                // Show each surviving deleted entry on its own — extent grouping
                // is unreliable once the user-number byte is gone.
                UniversalFile f;
                f.is_dir = false;
                f.is_deleted = true;
                f.name = make_file_name(*catalog[i]);
                f.size = catalog[i]->RC * 128;
                f.is_protected = (catalog[i]->E[0] & 0x80) != 0;
                f.type_label = "";
                f.type_label += (catalog[i]->E[1] & 0x80) ? "S" : "";
                f.type_label += (catalog[i]->E[2] & 0x80) ? "A" : "";

                f.attributes = 0xE5 << 8;

                std::string ext = get_file_ext(f.name);
                f.type_preferred = (txts.find(ext) != txts.end()) ? PreferredType::Text : PreferredType::Binary;
                if (ext == ".bas")
                    f.type_preferred = PreferredType::MBASIC;

                f.metadata.resize(sizeof(CPM_DIR_ENTRY));
                std::memcpy(f.metadata.data(), catalog[i], sizeof(CPM_DIR_ENTRY));

                files.push_back(f);
                continue;
            }

            // Skip CP/M 3 password entries (and other non-file slots).
            if (ST == 0x1F) continue;

            std::string file_name = make_file_name(*catalog[i]);
            const int extent = catalog[i]->XH*32 + catalog[i]->XL;
            if (extent == 0 || prev_name != file_name || prev_st != ST ) {
                prev_name = file_name;
                prev_st = ST;

                UniversalFile f;
                f.is_dir = false;
                f.is_deleted = false;
                f.name = file_name;
                f.size = catalog[i]->RC * 128;

                f.is_protected = (catalog[i]->E[0] & 0x80) != 0;
                f.type_label = "";
                f.type_label += (catalog[i]->E[1] & 0x80)?"S":""; // System (hidden)
                f.type_label += (catalog[i]->E[2] & 0x80)?"A":""; // Archived
                f.attributes = ST << 8;

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
            }
        }

        return Result::ok();
    }

    Result fsCPM::delete_file(const UniversalFile & uf)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (uf.metadata.size() < sizeof(CPM_DIR_ENTRY))
            return Result::error(ErrorCode::FileDeleteError);

        const auto * file_entry = reinterpret_cast<const CPM_DIR_ENTRY *>(uf.metadata.data());

        const int catalog_size = DPB.DRM + 1;
        const int entries_in_sector = static_cast<int>(image->get_sector_size() / sizeof(CPM_DIR_ENTRY));
        const int directory_sectors = catalog_size / entries_in_sector;

        bool found = false;

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            if (!sector) return Result::error(ErrorCode::FileDeleteError);

            for (int j = 0; j < entries_in_sector; j++) {
                auto * de = reinterpret_cast<CPM_DIR_ENTRY *>(sector + j * sizeof(CPM_DIR_ENTRY));

                // Skip already-deleted entries and entries from other user areas
                if (de->ST == 0xE5 || de->ST != file_entry->ST) continue;

                // Match by 8-byte name
                if (std::memcmp(de->F, file_entry->F, 8) != 0) continue;

                // Match by 3-byte extension, masking off the attribute bits (R/O, System, Archive)
                bool ext_match = true;
                for (int k = 0; k < 3; k++) {
                    if ((de->E[k] & 0x7F) != (file_entry->E[k] & 0x7F)) {
                        ext_match = false;
                        break;
                    }
                }
                if (!ext_match) continue;

                de->ST = 0xE5;
                found = true;
            }
        }

        if (!found) return Result::error(ErrorCode::FileDeleteError);

        is_changed = true;
        return Result::ok();
    }

    std::pair<std::string, std::string> fsCPM::exattr_caption()
    {
        return {"U", "User #"};
    }

    std::string fsCPM::exattr(const UniversalFile & fd)
    {
        const unsigned u = (fd.attributes >> 8) & 0xFF;
        return (u != 0xE5) ? std::to_string(u) : std::string("-");
    }

    std::vector<ParameterDescription> fsCPM::file_get_metadata(const UniversalFile & fd)
    {
        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"user",    "{$META_CPM_USER}",  ParamType::Byte,     std::to_string(fd.attributes >> 8)});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        if (fd.metadata.size() >= sizeof(CPM_DIR_ENTRY)) {
            const auto * de = reinterpret_cast<const CPM_DIR_ENTRY *>(fd.metadata.data());
            params.push_back({"system",  "{$META_CPM_SYSTEM}",  ParamType::Checkbox, (de->E[1] & 0x80) ? "true" : "false"});
            params.push_back({"archive", "{$META_CPM_ARCHIVE}", ParamType::Checkbox, (de->E[2] & 0x80) ? "true" : "false"});
        }

        return params;
    }

    Result fsCPM::rename_file(const UniversalFile & fd, const std::string & new_name)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (fd.metadata.size() < sizeof(CPM_DIR_ENTRY))
            return Result::error(ErrorCode::FileRenameError);

        // Split "BASE.EXT" — base ≤ 8, ext ≤ 3 (CP/M 8.3 filename).
        std::string base, ext;
        const auto dot = new_name.find_last_of('.');
        if (dot == std::string::npos) {
            base = new_name;
        } else {
            base = new_name.substr(0, dot);
            ext  = new_name.substr(dot + 1);
        }

        if (base.empty() || base.size() > 8 || ext.size() > 3)
            return Result::error(ErrorCode::InvalidName);

        base = to_upper(base);
        ext  = to_upper(ext);

        uint8_t new_F[8];
        std::memset(new_F, ' ', sizeof(new_F));
        std::memcpy(new_F, base.data(), base.size());

        uint8_t new_E[3];
        std::memset(new_E, ' ', sizeof(new_E));
        std::memcpy(new_E, ext.data(), ext.size());

        const auto * file_entry = reinterpret_cast<const CPM_DIR_ENTRY *>(fd.metadata.data());

        const int catalog_size = DPB.DRM + 1;
        const int entries_in_sector = static_cast<int>(image->get_sector_size() / sizeof(CPM_DIR_ENTRY));
        const int directory_sectors = catalog_size / entries_in_sector;

        bool found = false;

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            if (!sector) return Result::error(ErrorCode::FileRenameError);

            for (int j = 0; j < entries_in_sector; j++) {
                auto * de = reinterpret_cast<CPM_DIR_ENTRY *>(sector + j * sizeof(CPM_DIR_ENTRY));

                if (de->ST == 0xE5 || de->ST != file_entry->ST) continue;
                if (std::memcmp(de->F, file_entry->F, 8) != 0) continue;

                bool ext_match = true;
                for (int k = 0; k < 3; k++) {
                    if ((de->E[k] & 0x7F) != (file_entry->E[k] & 0x7F)) {
                        ext_match = false;
                        break;
                    }
                }
                if (!ext_match) continue;

                // Overwrite name; preserve the attribute bits (R/O, System, Archive) stored
                // in the high bit of each E byte.
                std::memcpy(de->F, new_F, sizeof(new_F));
                for (int k = 0; k < 3; k++)
                    de->E[k] = (de->E[k] & 0x80) | (new_E[k] & 0x7F);

                found = true;
            }
        }

        if (!found) return Result::error(ErrorCode::FileRenameError);

        is_changed = true;
        return Result::ok();
    }

    Result fsCPM::file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (fd.metadata.size() < sizeof(CPM_DIR_ENTRY))
            return Result::error(ErrorCode::FileMetadataError);

        const auto * file_entry = reinterpret_cast<const CPM_DIR_ENTRY *>(fd.metadata.data());

        // Collect intended changes; only mark "change_*" if value actually differs.
        bool change_name = false;
        std::string new_name_str;

        bool change_user = false;
        uint8_t new_ST = file_entry->ST;

        const bool cur_protected = (file_entry->E[0] & 0x80) != 0;
        const bool cur_system    = (file_entry->E[1] & 0x80) != 0;
        const bool cur_archive   = (file_entry->E[2] & 0x80) != 0;

        bool change_protected = false, new_protected = cur_protected;
        bool change_system    = false, new_system    = cur_system;
        bool change_archive   = false, new_archive   = cur_archive;

        for (const auto & p : metadata) {
            const std::string & key = p.first;
            const std::string & val = p.second;

            if (key == "filename") {
                if (val != fd.name) {
                    change_name = true;
                    new_name_str = val;
                }
            } else if (key == "user") {
                const int u = std::stoi(val);
                if (u < 0 || u > 0x1F)
                    return Result::error(ErrorCode::FileMetadataError, "user number out of range");
                new_ST = static_cast<uint8_t>(u);
                change_user = (new_ST != file_entry->ST);
            } else if (key == "protected") {
                new_protected = (val == "true");
                change_protected = (new_protected != cur_protected);
            } else if (key == "system") {
                new_system = (val == "true");
                change_system = (new_system != cur_system);
            } else if (key == "archive") {
                new_archive = (val == "true");
                change_archive = (new_archive != cur_archive);
            }
        }

        // Step 1: apply ST + attribute-bit changes by scanning the catalog.
        // Match by the *original* user + name + extension stored in fd.metadata.
        if (change_user || change_protected || change_system || change_archive) {
            const int catalog_size = DPB.DRM + 1;
            const int entries_in_sector = static_cast<int>(image->get_sector_size() / sizeof(CPM_DIR_ENTRY));
            const int directory_sectors = catalog_size / entries_in_sector;

            bool found = false;

            for (int i = 0; i < directory_sectors; i++) {
                uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
                if (!sector) return Result::error(ErrorCode::FileMetadataError);

                for (int j = 0; j < entries_in_sector; j++) {
                    auto * de = reinterpret_cast<CPM_DIR_ENTRY *>(sector + j * sizeof(CPM_DIR_ENTRY));

                    if (de->ST == 0xE5 || de->ST != file_entry->ST) continue;
                    if (std::memcmp(de->F, file_entry->F, 8) != 0) continue;

                    bool ext_match = true;
                    for (int k = 0; k < 3; k++) {
                        if ((de->E[k] & 0x7F) != (file_entry->E[k] & 0x7F)) {
                            ext_match = false;
                            break;
                        }
                    }
                    if (!ext_match) continue;

                    if (change_user)      de->ST = new_ST;
                    if (change_protected) de->E[0] = (de->E[0] & 0x7F) | (new_protected ? 0x80 : 0x00);
                    if (change_system)    de->E[1] = (de->E[1] & 0x7F) | (new_system    ? 0x80 : 0x00);
                    if (change_archive)   de->E[2] = (de->E[2] & 0x7F) | (new_archive   ? 0x80 : 0x00);

                    found = true;
                }
            }

            if (!found) return Result::error(ErrorCode::FileMetadataError);
            is_changed = true;
        }

        // Step 2: rename via rename_file(). Build a temp UniversalFile whose metadata
        // reflects the post-step-1 catalog state so rename_file's match key
        // (ST + F + E_low7) still hits the same entries — only ST may have shifted.
        if (change_name) {
            UniversalFile tmp = fd;
            if (change_user) {
                auto * tmp_entry = reinterpret_cast<CPM_DIR_ENTRY *>(tmp.metadata.data());
                tmp_entry->ST = new_ST;
            }
            const auto res = rename_file(tmp, new_name_str);
            if (!res) return res;
        }

        return Result::ok();
    }

    Result fsCPM::put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        // ---- Parse and validate the destination 8.3 filename
        const std::string filename = get_filename(uf.name);
        std::string base, ext;
        const auto dot = filename.find_last_of('.');
        if (dot == std::string::npos) {
            base = filename;
        } else {
            base = filename.substr(0, dot);
            ext  = filename.substr(dot + 1);
        }
        if (base.empty() || base.size() > 8 || ext.size() > 3)
            return Result::error(ErrorCode::InvalidName);
        base = to_upper(base);
        ext  = to_upper(ext);

        uint8_t name_F[8];
        uint8_t name_E[3];
        std::memset(name_F, ' ', sizeof(name_F));
        std::memcpy(name_F, base.data(), base.size());
        std::memset(name_E, ' ', sizeof(name_E));
        std::memcpy(name_E, ext.data(), ext.size());

        constexpr uint8_t user_no = 0; // default user area

        // ---- Disk geometry from DPB
        const int sector_size  = static_cast<int>(image->get_sector_size());
        const int BLS          = 1 << (DPB.BSH + 7);
        const int spb          = BLS / sector_size;
        const int sectors      = static_cast<int>(image->get_sectors());
        const int heads        = static_cast<int>(image->get_heads());
        const int index_shift  = DPB.OFF * sectors * heads;
        const int total_blocks = DPB.DSM + 1;
        const int catalog_size = DPB.DRM + 1;
        const int entries_in_sector = static_cast<int>(sector_size / sizeof(CPM_DIR_ENTRY));
        const int directory_sectors = catalog_size / entries_in_sector;

        // CP/M 2.x with DSM<256: 16 single-byte AL entries per extent (16 blocks).
        // CP/M with DSM>=256: 8 word AL entries per extent (8 blocks).
        const bool al_16bit = (DPB.DSM >= 256);
        const int al_entries_per_extent = al_16bit ? 8 : 16;
        const int blocks_per_extent_max = al_entries_per_extent;
        const int records_per_extent_max = (blocks_per_extent_max * BLS) / 128; // 128-byte records

        // ---- Walk catalog: cache pointers, find existing target file, build used-block map
        std::vector<CPM_DIR_ENTRY*> catalog(catalog_size);
        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, translate_sector(i));
            if (!sector) return Result::error(ErrorCode::WriteError);
            for (int j = 0; j < entries_in_sector; j++)
                catalog[i*entries_in_sector + j] = reinterpret_cast<CPM_DIR_ENTRY*>(sector + j*sizeof(CPM_DIR_ENTRY));
        }

        std::vector<bool> block_used(total_blocks, false);

        // Reserve directory blocks per AL0:AL1 (top bits = first blocks).
        {
            const uint16_t reserved = (static_cast<uint16_t>(DPB.AL0) << 8) | DPB.AL1;
            for (int b = 0; b < 16 && b < total_blocks; b++)
                if (reserved & (1u << (15 - b)))
                    block_used[b] = true;
        }

        std::vector<int> target_entries; // catalog indexes belonging to target file (live)
        for (int i = 0; i < catalog_size; i++) {
            const auto * de = catalog[i];
            if (de->ST == 0xE5 || de->ST == 0x1F) continue;

            // Match against target?
            bool is_target = (de->ST == user_no) && (std::memcmp(de->F, name_F, 8) == 0);
            if (is_target) {
                for (int k = 0; k < 3; k++) {
                    if ((de->E[k] & 0x7F) != name_E[k]) { is_target = false; break; }
                }
            }
            if (is_target) target_entries.push_back(i);

            // Mark blocks (for non-target entries — target's blocks are about to be freed).
            if (is_target) continue;
            for (int idx = 0; idx < al_entries_per_extent; idx++) {
                uint16_t blk = al_16bit
                    ? static_cast<uint16_t>(de->AL[idx*2] | (de->AL[idx*2+1] << 8))
                    : static_cast<uint16_t>(de->AL[idx]);
                if (blk != 0 && blk < total_blocks) block_used[blk] = true;
            }
        }

        if (!target_entries.empty() && !force_replace)
            return Result::error(ErrorCode::FileAlreadyExists);

        // ---- Compute requirements
        const size_t file_size = data.size();
        const int records_total = static_cast<int>((file_size + 127) / 128);
        const int blocks_needed = static_cast<int>((file_size + BLS - 1) / BLS);
        const int extents_needed = (records_total == 0)
            ? 1
            : (records_total + records_per_extent_max - 1) / records_per_extent_max;

        // Free entries = currently-deleted slots + target slots that we'll release.
        int free_entries = static_cast<int>(target_entries.size());
        for (int i = 0; i < catalog_size; i++)
            if (catalog[i]->ST == 0xE5) free_entries++;
        if (free_entries < extents_needed)
            return Result::error(ErrorCode::FileAddErrorAllocateDirEntry);

        int free_blocks_count = 0;
        for (int b = 0; b < total_blocks; b++)
            if (!block_used[b]) free_blocks_count++;
        if (free_blocks_count < blocks_needed)
            return Result::error(ErrorCode::FileAddErrorSpace);

        // ---- Commit: release the old file's directory entries (blocks were already
        // excluded from block_used, so allocation will reuse them).
        for (int idx : target_entries)
            catalog[idx]->ST = 0xE5;

        // ---- Allocate blocks (lowest-first)
        std::vector<uint16_t> alloc_blocks;
        alloc_blocks.reserve(blocks_needed);
        for (int b = 0; b < total_blocks && static_cast<int>(alloc_blocks.size()) < blocks_needed; b++) {
            if (!block_used[b]) {
                alloc_blocks.push_back(static_cast<uint16_t>(b));
                block_used[b] = true;
            }
        }

        // ---- Write data into the allocated blocks
        for (int blk_idx = 0; blk_idx < blocks_needed; blk_idx++) {
            const uint16_t blk = alloc_blocks[blk_idx];
            for (int s = 0; s < spb; s++) {
                const int sector_index = blk * spb + s + index_shift;
                int head, track;
                if (heads == 1) {
                    head = 0;
                    track = sector_index / sectors;
                } else {
                    head = (sector_index / sectors) & 1;
                    track = (sector_index / sectors) >> 1;
                }
                const int sector = translate_sector(sector_index % sectors);
                uint8_t * disk_data = image->get_sector_data(head, track, sector);
                if (!disk_data) return Result::error(ErrorCode::WriteError);

                const size_t offset = static_cast<size_t>(blk_idx) * BLS + s * sector_size;
                if (offset >= file_size) {
                    // Trailing sector beyond data — pad with CP/M EOF (0x1A).
                    std::memset(disk_data, 0x1A, sector_size);
                } else {
                    const size_t remaining = file_size - offset;
                    const auto   ss        = static_cast<size_t>(sector_size);
                    const size_t to_copy   = (remaining < ss) ? remaining : ss;
                    std::memcpy(disk_data, data.data() + offset, to_copy);
                    if (to_copy < ss)
                        std::memset(disk_data + to_copy, 0x1A, ss - to_copy);
                }
            }
        }

        // ---- Write directory extents
        int blocks_consumed  = 0;
        int records_consumed = 0;
        int next_free_entry  = 0;
        for (int ext_no = 0; ext_no < extents_needed; ext_no++) {
            while (next_free_entry < catalog_size && catalog[next_free_entry]->ST != 0xE5)
                next_free_entry++;
            if (next_free_entry >= catalog_size)
                return Result::error(ErrorCode::FileAddErrorAllocateDirEntry);

            auto * de = catalog[next_free_entry];
            std::memset(de, 0, sizeof(CPM_DIR_ENTRY));

            de->ST = user_no;
            std::memcpy(de->F, name_F, 8);
            std::memcpy(de->E, name_E, 3);
            de->BC = 0;
            de->XL = static_cast<uint8_t>(ext_no & 0x1F);
            de->XH = static_cast<uint8_t>((ext_no >> 5) & 0x3F);

            const int records_in_ext = std::min(records_per_extent_max, records_total - records_consumed);
            const int blocks_in_ext  = std::min(blocks_per_extent_max,  blocks_needed  - blocks_consumed);
            de->RC = static_cast<uint8_t>(records_in_ext);

            for (int idx = 0; idx < al_entries_per_extent; idx++) {
                const uint16_t blk = (idx < blocks_in_ext) ? alloc_blocks[blocks_consumed + idx] : 0;
                if (al_16bit) {
                    de->AL[idx*2]     = static_cast<uint8_t>(blk & 0xFF);
                    de->AL[idx*2 + 1] = static_cast<uint8_t>((blk >> 8) & 0xFF);
                } else {
                    de->AL[idx] = static_cast<uint8_t>(blk);
                }
            }

            records_consumed += records_in_ext;
            blocks_consumed  += blocks_in_ext;
            next_free_entry++;
        }

        is_changed = true;
        return Result::ok();
    }

}
