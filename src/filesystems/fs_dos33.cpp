// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the Apple DOS 3.3 filesystem

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_dos33.h"

namespace dsk_tools {

    fsDOS33::fsDOS33(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsDOS33::get_caps()
    {
        return  FSCaps::Protect | FSCaps::Types  | FSCaps::Delete | FSCaps::Add
                | FSCaps::Dirs  | FSCaps::Rename | FSCaps::MkDir  | FSCaps::Metadata
                | FSCaps::Restore | FSCaps::Export;
    }

    Result fsDOS33::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        uint8_t* vtoc_data = image->get_sector_data(0, 0x11, 0);
        if (!vtoc_data) return Result::error(ErrorCode::OpenBadFormat, "Cannot read VTOC");

        VTOC = reinterpret_cast<dsk_tools::Agat_VTOC *>(vtoc_data);
        const int sector_size = static_cast<int>(VTOC->bytes_per_sector);

        // Also: https://retrocomputing.stackexchange.com/questions/15054/how-can-i-programmatically-determine-whether-an-apple-ii-dsk-disk-image-is-a-do
        if (VTOC->sectors_on_track != image->get_sectors() || sector_size != 256) {
            return Result::error(ErrorCode::OpenBadFormat, "VTOC sector count or size mismatch");
        }

        const TS_PAIR root_ts = {
            VTOC->catalog_track,
            VTOC->catalog_sector
        };

        current_path.push_back(root_ts);

        is_open = true;
        volume_id = VTOC->volume_id;
        return Result::ok();
    }

    int fsDOS33::attr_to_type(const uint8_t a)
    {
        uint8_t v = a & 0x7F;
        int n = 0;
        do {
            if (v == 0) return n;
            v >>= 1;
            n++;
        } while (n < 8);
        return 0;
    }

    void fsDOS33::cd_up()
    {
        if (current_path.size() > 1) current_path.pop_back();
    }

    void fsDOS33::cd(const dsk_tools::UniversalFile & dir, bool & updir)
    {
        if (dir.name == "..") {
            cd_up();
            updir = true;
        } else {
            Apple_DOS_File_Metadata f{};
            memcpy(&f, dir.metadata.data(), sizeof(f));

            // std::cout << "CD: " << (int)f.tbl_track << ":" << (int)f.tbl_sector << std::endl;

            current_path.push_back(
                {
                    f.dir_entry.tbl_track,
                    f.dir_entry.tbl_sector
                }
            );
            updir = false;
        }
    }

    std::string fsDOS33::file_info(const UniversalFile & fd) {
        if (fd.metadata.size() < sizeof(Apple_DOS_File_Metadata)) {
            return "{$ERROR_INVALID_METADATA}";
        }
        const auto * metadata = reinterpret_cast<const Apple_DOS_File_Metadata *>(fd.metadata.data());
        const auto * dir_entry = &metadata->dir_entry;

        std::string result;
        result += "{$DIRECTORY_ENTRY}:\n";
        result += "    {$FILE_NAME}: " +  trim(agat_to_utf(dir_entry->name, 30)) + " (" + toHexList(dir_entry->name, 30, "$") +")\n";
        result += "    {$SIZE}: " + std::to_string(dir_entry->size * 256) + " {$BYTES} (" + std::to_string(dir_entry->size) + " {$SECTORS})\n";
        result += "    {$TYPE}: " + std::string(agat_file_types[attr_to_type(dir_entry->type)]) + " ($" + int_to_hex(dir_entry->type) + ")\n";
        result += "    {$PROTECTED}: " + (((dir_entry->type & 0x80) > 0)?std::string("{$YES}"):std::string("{$NO}")) + "\n";

        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

        if (list_track == 0xFF) {
            result += "\n{$FILE_DELETED}:\n";
            list_track = dir_entry->name[29];
            result += "    {$TS_LIST_LOCATION}: " + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
        } else {
            result += "    {$TS_LIST_LOCATION}: " + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
        }

        Apple_DOS_TS_List * ts_list;

        uint8_t* ts_list_data = image->get_sector_data(0, list_track, list_sector);
        if (!ts_list_data) return "{$ERROR_CANNOT_READ_TS_LIST}";

        ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(ts_list_data);
        BYTES ts_custom(fd.is_dir?8:9);
        void * from = &(ts_list->_not_used_03);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        result += "    {$CUSTOM_DATA}: [" +  toHexList(ts_custom, "$") +"]\n";

        result += "\n";

        BYTES buffer;
        auto res =  get_file(fd, "", buffer);
        result += agat_vr_info(buffer);

        result += "{$TS_LIST_DATA}:\n";
        do {
            result += "T/S "  + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
            if (list_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                uint8_t* ts_list_data2 = image->get_sector_data(0, list_track, list_sector);
                if (!ts_list_data2) {
                    result += "{$ERROR_CANNOT_READ_TS_LIST}!!!\n";
                    break;
                }
                ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(ts_list_data2);


                for (int i = 0; i < VTOC->pairs_on_sector; i++){
                    int file_track = ts_list->ts[i][0];
                    int file_sector = ts_list->ts[i][1];
                    result += "    "  + std::to_string(file_track) + ":" + std::to_string(file_sector) + "\n";

                    if (file_track == 0) break;
                }


                list_track = ts_list->next_track;
                list_sector = ts_list->next_sector;
                if (list_track != 0 || list_sector != 0) {
                    result += "    {$NEXT_TS}: "  + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
                } else {
                    result += "{$FILE_END_REACHED}";
                }
            } else {
                result += "{$INCORRECT_TS_DATA}!!!\n";
                break;
            }

        } while (list_track != 0);

        return result;
    }

    std::vector<std::string> fsDOS33::get_save_file_formats()
    {
        return {"FILE_FIL", "FILE_BINARY"};
    }

    std::vector<std::string> fsDOS33::get_add_file_formats()
    {
        return {"FILE_FIL", "FILE_ANY"};
    }

    std::string fsDOS33::information()
    {
        std::string result;
        result += agat_vtoc_info(*VTOC);
        result += "\n";

        result += "{$FREE_SECTORS}: "  + std::to_string(free_sectors()) + "\n";
        result += "{$FREE_BYTES}: "  + std::to_string(free_sectors()*256) + "\n";

        return result;
    }

    uint32_t * fsDOS33::track_map(const int track)
    {
        Agat_VTOC_Ex * VTOCEx;
        const int tracks_count = image->get_tracks()*image->get_heads();
        if (track < 0x32)
            return &(VTOC->free_sectors[track]);
        else
        if (track < 0x72 && tracks_count > 0x32) {
            uint8_t* vtoc_ex_data = image->get_sector_data(0, 0x32, 0);
            if (!vtoc_ex_data)
                throw std::runtime_error("Cannot read VTOC extension sector (0x32, 0)");

            VTOCEx = reinterpret_cast<Agat_VTOC_Ex *>(vtoc_ex_data);
            return &(VTOCEx->free_sectors[track-0x32]);
        } else
        if (track < 0xB2 && tracks_count > 0x72) {
            uint8_t* vtoc_ex_data = image->get_sector_data(0, 0x72, 0);
            if (!vtoc_ex_data)
                throw std::runtime_error("Cannot read VTOC extension sector (0x72, 0)");

            VTOCEx = reinterpret_cast<Agat_VTOC_Ex *>(vtoc_ex_data);
            return &(VTOCEx->free_sectors[track-0x72]);
        } else
            throw std::runtime_error("Incorrect track");
    }

    int fsDOS33::free_sectors()
    {
        int free_sectors = 0;
        for (int track=0; track < image->get_tracks()*image->get_heads(); track ++)
            for (int sector=0; sector<image->get_sectors(); sector++)
                free_sectors += sector_is_free(0, track, sector);
        return free_sectors;
    }

    bool fsDOS33::sector_is_free(int head, int track, int sector)
    {
        // std::cout << "? " << track << ":" << sector << std::endl;

        if (image->get_sectors() == 16)
            return *track_map(track) & VTOCMask140[sector];
        else
        if (image->get_sectors() == 21)
            return *track_map(track) & VTOCMask840[sector];
        else
            throw std::runtime_error("Incorrect disk type");
    }

    void fsDOS33::sector_free(int head, int track, int sector)
    {
        // std::cout << "sector_free: " << track << ":" << sector << std::endl;

        *track_map(track) |= (image->get_sectors()==16)?VTOCMask140[sector]:VTOCMask840[sector];
    }

    bool fsDOS33::sector_occupy(int head, int track, int sector)
    {
        // std::cout << "sector_occupy: " << track << ":" << sector << std::endl;

        if (!sector_is_free(0, track, sector)) return false;
        *track_map(track) &= ~((image->get_sectors()==16)?VTOCMask140[sector]:VTOCMask840[sector]);
        return true;
    }

    bool fsDOS33::find_empty_sector(uint8_t start_track, TS_PAIR &ts, bool go_forward)
    {
        // std::cout << "find_empty_sector: start_track=" << (int)start_track << std::endl;

        int track = start_track;
        int sector;

        do {
            int inc;
            if (go_forward) {
                sector = 0;
                inc = 1;
            } else {
                sector = image->get_sectors()-1;
                inc = -1;
            }

            while (sector>=0 && sector<image->get_sectors() && !sector_is_free(0, track, sector)) sector += inc;

            if (sector >=0 && sector < image->get_sectors()) {
                ts.track = track;
                ts.sector = sector;

                // std::cout << "==> found T:S = " << track << ":" << sector << std::endl;

                return true;
            } else {
                track += (track >= start_track)?1:-1;
                if (track < 0 || track >= image->get_tracks())
                    return false;
            }
        } while (true);
    }

    bool fsDOS33::find_epmty_dir_entry(Apple_DOS_File *& dir_entry, int & dir_pos, bool just_check, bool &extra_sector)
    {

        TS_PAIR catalog_ts = current_path.back();
        Apple_DOS_Catalog * catalog;
        TS_PAIR last_ts{};

        do {
            uint8_t* catalog_data = image->get_sector_data(0, catalog_ts.track, catalog_ts.sector);
            if (!catalog_data) {
                return false;  // Cannot read catalog sector
            }
            catalog = reinterpret_cast<Apple_DOS_Catalog *>(catalog_data);
            // std::cout << "find_epmty_dir_entry" <<  std::endl;
            // std::cout << "==> catalog T:S = " << unsigned(catalog_ts.track) << ":" << unsigned(catalog_ts.sector) << std::endl;

            for (int i=0; i<7; i++) {
                const uint8_t tbl_track = catalog->files[i].tbl_track;
                if (tbl_track == 0xFF || tbl_track == 0x00) {
                    dir_entry = &(catalog->files[i]);
                    dir_pos = i;
                    extra_sector = false;
                    // std::cout << "==> found position: " << i << std::endl;
                    return true;
                }
            }

            last_ts = catalog_ts;
            catalog_ts.track = catalog->next_track;
            catalog_ts.sector = catalog->next_sector;
        } while (catalog_ts.track != 0);
        extra_sector = true;
        if (!just_check) {
            // Occupy a new directory sector
            TS_PAIR new_ts{};
            const bool res = find_empty_sector(last_ts.track, new_ts, true);
            if (res) {
                sector_occupy(0, new_ts.track, new_ts.sector);
                catalog->next_track = new_ts.track;
                catalog->next_sector = new_ts.sector;

                uint8_t* new_catalog_data = image->get_sector_data(0, new_ts.track, new_ts.sector);
                if (!new_catalog_data) {
                    return false;  // Cannot read new catalog sector
                }
                auto * new_catalog = reinterpret_cast<Apple_DOS_Catalog *>(new_catalog_data);
                std::memset(new_catalog, 0, sizeof(Apple_DOS_Catalog));
                dir_entry = &(new_catalog->files[0]);
                return true;
            } else
                return false;
        }
        return true;
    }

    Result fsDOS33::mkdir(const std::string & dir_name,  UniversalFile & new_dir)
    {
        UniversalFile uf;
        uf.fs = FS::None;
        uf.name = dir_name;
        return mkdir(uf, new_dir);
    }

    Result fsDOS33::mkdir(const UniversalFile & uf,  UniversalFile & new_dir)
    {
        // std::cout << "mkdir: " << uf.name << std::endl;

        constexpr int sectors_body = 1;  // We need at least 1 sector for the new directory

        // Check if we need a new sector for catalog
        Apple_DOS_File * dir_entry;
        bool extra_sector;
        int dir_pos;
        if (!find_epmty_dir_entry(dir_entry, dir_pos, true, extra_sector))
            return Result::error(ErrorCode::DirErrorAllocateDirEntry);
        const int sectors_catalog = (extra_sector)?1:0;

        int sectors_total = sectors_body + sectors_catalog;

        if (sectors_total > free_sectors())
            return Result::error(ErrorCode::DirErrorSpace);

        // Create a directory entry
        if (!find_epmty_dir_entry(dir_entry, dir_pos, false, extra_sector))
            return Result::error(ErrorCode::DirErrorAllocateDirEntry);

        // Metadata
        std::memset(dir_entry, 0, sizeof(Apple_DOS_File));
        dir_entry->type = 0xFF;
        dir_entry->size = sectors_total;

        // Name
        if (uf.fs == FS::DOS33) {
            // For native FS we take an original filename
            const auto * metadata = reinterpret_cast<const Apple_DOS_File_Metadata *>(uf.metadata.data());
            std::memcpy(dir_entry->name, &metadata->dir_entry, sizeof(dir_entry->name));
        } else {
            // For other cases we convert from universal name
            std::memset(dir_entry->name, 0xA0, sizeof(dir_entry->name));
            const BYTES name_str = utf_to_agat(uf.name);
            const auto len = name_str.size();
            std::memcpy(dir_entry->name, name_str.data(), (len <= sizeof(dir_entry->name))?len:sizeof(dir_entry->name));
        }

        // Catalog body
        const TS_PAIR catalog_ts = current_path.back();
        TS_PAIR ts{};
        const bool res = find_empty_sector(catalog_ts.track, ts, false);
        if (!res)
            return Result::error(ErrorCode::DirErrorAllocateSector);;

        // std::cout << "catalog position T:S = " << (int)ts.track << ":" << (int)ts.sector << std::endl;

        sector_occupy(0, ts.track, ts.sector);

        auto * new_catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, ts.track, ts.sector));
        if (!new_catalog) return Result::error(ErrorCode::DirErrorAllocateSector);

        std::memset(new_catalog, 0, sizeof(Apple_DOS_Catalog));

        dir_entry->tbl_track = ts.track;
        dir_entry->tbl_sector = ts.sector;

        // Reverse link to the parent
        std::memset(&(new_catalog->files[0]), 0, sizeof(Apple_DOS_File));
        new_catalog->files[0].tbl_track = catalog_ts.track;
        new_catalog->files[0].tbl_sector = catalog_ts.sector;
        new_catalog->files[0].type = 0xFF;
        std::memcpy(new_catalog->files[0].name, &dir_entry->name, sizeof(dir_entry->name));

        new_dir.fs = FS::DOS33;
        new_dir.name = uf.name;
        new_dir.is_dir = true;

        Apple_DOS_File_Metadata metadata {};
        std::memcpy(&metadata.dir_entry, dir_entry, sizeof(metadata.dir_entry));
        std::memset(metadata.tsl, 0, sizeof(metadata.tsl));
        new_dir.metadata.resize(sizeof(Apple_DOS_File_Metadata));
        memcpy(new_dir.metadata.data(), &metadata, sizeof(metadata));

        new_dir.position.push_back(catalog_ts.track);
        new_dir.position.push_back(catalog_ts.sector);
        new_dir.position.push_back(dir_pos);

        is_changed = true;
        return Result::ok();
    }

    bool fsDOS33::is_root()
    {
        return current_path.size() == 1;
    }

    Result fsDOS33::find_file(const std::string & file_name, UniversalFile & fd)
    {
        Files files;
        dir(files, false);
        for (const UniversalFile& f : files) {
            if (to_upper(f.name) == file_name) {
                fd = f;
                return Result::ok();
            }
        }
        return Result::error(ErrorCode::NotFound);
    }

    Result fsDOS33::get_file_contents(const Apple_DOS_File * dir_entry, BYTES & data) const {
        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

        if (list_track == 0xFF) {
            list_track = dir_entry->name[29];
        }

        do {
            const auto * ts_list = reinterpret_cast<const Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
            if (!ts_list) return Result::error(ErrorCode::ReadError);

            for (int i = 0; i < VTOC->pairs_on_sector; i++){
                const int file_track = ts_list->ts[i][0];
                const int file_sector = ts_list->ts[i][1];
                if (file_track == 0) break;
                std::uint8_t * sector = image->get_sector_data(0, file_track, file_sector);
                if (!sector) return Result::error(ErrorCode::ReadError);
                data.insert(data.end(),&sector[0],&sector[256]);
            }

            list_track = ts_list->next_track;
            list_sector = ts_list->next_sector;

        } while (list_track != 0);

        return Result::ok();

    }

    Result fsDOS33::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (uf.fs != FS::DOS33) return Result::error(ErrorCode::FileIncorrectFS);

        data.clear();

        const auto *dir_entry = reinterpret_cast<const Apple_DOS_File *>(uf.metadata.data());

        if (format == "FILE_FIL") {
            FIL_header header {};
            memcpy(&header.name, &(dir_entry->name), sizeof(header.name));
            header.type = dir_entry->type;
            if (attr_to_type(dir_entry->type) == 6) {
                // K Type
                // http://agatcomp.ru/agat/PCutils/FileType/FIL.shtml
                const int list_track = dir_entry->tbl_track;
                const int list_sector = dir_entry->tbl_sector;

                if (list_track != 0xFF) {
                    auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                    if (!ts_list) return Result::error(ErrorCode::ReadError);
                    const void * from = &(ts_list->_not_used_03);
                    std::memcpy(header.tsl, from, 9);
                } else
                    std::memset(header.tsl, 0, sizeof(header.tsl));
            } else
                std::memset(header.tsl, 0, sizeof(header.tsl));

            const auto* headerBytes = reinterpret_cast<const uint8_t*>(&header);
            data.insert(data.end(), headerBytes, headerBytes + sizeof(header));
        } else
            if (format != "FILE_BINARY" && !format.empty())
                return Result::error(ErrorCode::WriteUnsupported);

        auto Result = get_file_contents(dir_entry, data);

        return Result::ok();
    }

    Result fsDOS33::put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace)
    {
        bool is_fil = false;
        bool is_native = false;
        if (format.empty()) {
            // Suggest format;
            if (uf.fs == FS::Host) {
                const std::string ext = get_file_ext(bytesToString(uf.metadata));
                is_fil = ext==".fil";
            } else if (uf.fs == FS::DOS33) {
                is_native = true;
            }
        }

        uint8_t file_type;
        uint8_t file_name[30];
        uint8_t file_tsl[9];
        int data_offset = 0;

        if (is_fil) {
            FIL_header header {};
            std::memcpy(&header, data.data(), sizeof(header));
            file_type = header.type;
            std::memcpy(file_name, header.name, sizeof(file_name));
            std::memcpy(file_tsl, header.tsl, sizeof(file_tsl));
            data_offset = sizeof(header);
        } else {
            if (is_native) {
                Apple_DOS_File_Metadata metadata {};
                std::memcpy(&metadata, uf.metadata.data(), std::min(uf.metadata.size(), sizeof(metadata)));
                file_type = metadata.dir_entry.type;
                std::memcpy(file_name, metadata.dir_entry.name, std::min(sizeof(file_name), sizeof(metadata.dir_entry.name)));
                std::memcpy(file_tsl, metadata.tsl, sizeof(file_tsl));
            } else {
                file_type = 0;
                std::memset(file_name, 0xA0, sizeof(file_name));
                const BYTES name_str = utf_to_agat(get_filename(uf.name));
                const auto len = name_str.size();
                std::memcpy(file_name, name_str.data(), (len <= sizeof(file_name))?len:sizeof(file_name));
                std::memset(file_tsl, 0, sizeof(file_tsl));
            }
        }

        const auto file_size = data.size() - data_offset;

        // --------------- Checking for sufficient space

        const auto sectors_body = (file_size+255)/256;                                           // File body
        const auto ts_pairs = sectors_body + 1;                                                  // We need an extra 0:0 pair to finish the list;
        const auto ts_lists = (ts_pairs + VTOC->pairs_on_sector - 1) / VTOC->pairs_on_sector;    // T/S lists, 122 sectors each.

        // Check if we need a new sector for catalog
        Apple_DOS_File * dir_entry;
        bool extra_sector;
        int dir_pos;
        if (!find_epmty_dir_entry(dir_entry, dir_pos, true, extra_sector))
            return Result::error(ErrorCode::FileAddErrorAllocateDirEntry);
        const auto sectors_catalog = (extra_sector)?1:0;
        const auto sectors_total = sectors_body + ts_lists + sectors_catalog;
        if (sectors_total > free_sectors())
            return Result::error(ErrorCode::FileAddErrorSpace);

        // ----------------   Main process

        // Create a directory entry
        if (!find_epmty_dir_entry(dir_entry, dir_pos, false, extra_sector))
            return Result::error(ErrorCode::FileAddErrorAllocateDirEntry);

        std::memset(dir_entry, 0, sizeof(Apple_DOS_File));
        dir_entry->type = file_type;
        dir_entry->size = sectors_body + ts_lists;
        std::memcpy(dir_entry->name, file_name, sizeof(file_name));

        Apple_DOS_TS_List * last_ts_list = nullptr;

        // Filling T/S lists
        for (int i=0; i < ts_lists; i++) {

            // std::cout << "TS List: " << i << std::endl;

            const TS_PAIR catalog_ts = current_path.back();
            TS_PAIR ts {};
            bool res = find_empty_sector(catalog_ts.track, ts, false);
            if (!res)
                return Result::error(ErrorCode::FileAddErrorAllocateSector);

            // std::cout << ">" << (int)ts.track << ":" << (int)ts.sector << std::endl;

            sector_occupy(0, ts.track, ts.sector);
            auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, ts.track, ts.sector));
            if (!ts_list) return Result::error(ErrorCode::WriteError);

            std::memset(ts_list, 0, sizeof(Apple_DOS_TS_List));
            if (i==0) {
                // Store first T/S to a catalog entry
                // std::cout << "First, store to dir" << std::endl;

                dir_entry->tbl_track = ts.track;
                dir_entry->tbl_sector = ts.sector;

                // Set TSL for the first T/S list
                void * to_ptr = &(ts_list->_not_used_03);
                std::memcpy(to_ptr, file_tsl, 9);

            } else {
                // Secondary T/S lists make a chain
                // std::cout << "Subsequent, store to previous" << std::endl;
                ts_list->offset = i * VTOC->pairs_on_sector;
                last_ts_list->next_track = ts.track;
                last_ts_list->next_sector = ts.sector;
            }
            uint8_t start_track = ts.track;
            for (int j=0; j < VTOC->pairs_on_sector; j++) {
                const auto ts_pair = i*VTOC->pairs_on_sector + j;
                if (ts_pair < ts_pairs-1) {
                    // File part
                    TS_PAIR file_ts {};
                    res = find_empty_sector(start_track, file_ts, false);
                    if (!res)
                        return Result::error(ErrorCode::FileAddErrorAllocateSector);

                    // std::cout << (int)file_ts.track << ":" << (int)file_ts.sector << std::endl;

                    sector_occupy(0, file_ts.track, file_ts.sector);
                    ts_list->ts[j][0] = file_ts.track;
                    ts_list->ts[j][1] = file_ts.sector;

                    uint8_t * disk_data = image->get_sector_data(0, file_ts.track, file_ts.sector);
                    if (!disk_data) return Result::error(ErrorCode::WriteError);

                    std::memcpy(disk_data, data.data() + data_offset + ts_pair * image->get_sector_size(), image->get_sector_size());

                    start_track = file_ts.track;
                } else {
                    // List end mark 0:0
                    ts_list->ts[j][0] = 0;
                    ts_list->ts[j][1] = 0;
                    break;
                }
            }
            last_ts_list = ts_list;
        }

        is_changed = true;
        return Result::ok();
    }

    Result fsDOS33::delete_file(const UniversalFile & uf)
    {
        if (!uf.is_dir) {
            // ----- File
            const auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, uf.position[0], uf.position[1]));
            if (!catalog) return Result::error(ErrorCode::FileDeleteError);
            auto * dir_entry = const_cast<Apple_DOS_File *>(&(catalog->files[uf.position[2]]));

            int list_track = dir_entry->tbl_track;
            int list_sector = dir_entry->tbl_sector;

            if (list_track == 0xFF) return Result::ok();

            do {
                if (list_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                    const auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                    if (!ts_list) return Result::error(ErrorCode::FileDeleteError);

                    for (int i = 0; i < VTOC->pairs_on_sector; i++){
                        const int file_track = ts_list->ts[i][0];
                        const int file_sector = ts_list->ts[i][1];
                        if (file_track == 0 && file_sector == 0) break;
                        sector_free(0, file_track, file_sector);
                    }
                    sector_free(0, list_track, list_sector);
                    list_track = ts_list->next_track;
                    list_sector = ts_list->next_sector;
                } else {
                    return Result::error(ErrorCode::FileDeleteError, "Incorrect track/sector data");
                }

            } while (list_track != 0);

            dir_entry->name[29] = dir_entry->tbl_track;
            dir_entry->tbl_track = 0xFF;

            is_changed = true;

            return Result::ok();
        } else {
            // ----- Directory

            // Checking if it is empty
            auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, uf.position[0], uf.position[1]));
            if (!catalog) return Result::error(ErrorCode::FileDeleteError);

            Apple_DOS_File * dir_entry = &catalog->files[uf.position[2]];

            TS_PAIR catalog_ts {};

            catalog_ts.track = dir_entry->tbl_track;
            catalog_ts.sector = dir_entry->tbl_sector;

            int files_count = 0;
            bool first_part = true;

            do {
                catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));
                if (!catalog) return Result::error(ErrorCode::FileDeleteError);

                for (int i=0; i<7; i++) {
                    uint8_t t = catalog->files[i].tbl_track;
                    const bool updir = (first_part && i==0 && catalog->files[i].type==0xFF);
                    if (!updir) {
                        if (t == 0) break;
                        if (t < 0xFF) files_count++;
                    }
                }

                catalog_ts.track = catalog->next_track;
                catalog_ts.sector = catalog->next_sector;
                first_part = false;

            } while (catalog_ts.track != 0);

            // std::cout << "CNT: " << files_count <<std::endl;

            if (files_count == 0) {
                // Do the deletion!
                catalog_ts.track = dir_entry->tbl_track;
                catalog_ts.sector = dir_entry->tbl_sector;
                do {
                    catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));
                    if (!catalog) return Result::error(ErrorCode::FileDeleteError);

                    sector_free(0, catalog_ts.track, catalog_ts.sector);
                    catalog_ts.track = catalog->next_track;
                    catalog_ts.sector = catalog->next_sector;
                } while (catalog_ts.track != 0);

                dir_entry->name[29] = dir_entry->tbl_track;
                dir_entry->tbl_track = 0xFF;
                is_changed = true;
            } else
                return Result::error(ErrorCode::DirNotEmpty);
        }
        return Result::ok();
    }

    Result fsDOS33::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        TS_PAIR catalog_ts = current_path.back();

        // std::cout << "DIR: " << (int)catalog_ts.track << ":" << (int)catalog_ts.sector << std::endl;

        do {
            const auto catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));
            if (!catalog) return Result::error(ErrorCode::ReadError);

            // std::cout << "CATALOG: " << (int)catalog_ts.track << ":" << (int)catalog_ts.sector << std::endl;

            for (int i=0; i<7; i++) {
                const bool is_deleted = catalog->files[i].tbl_track == 0xFF;
                if (!is_deleted || show_deleted) {
                    if (catalog->files[i].tbl_track == 0) {
                        // Means end of the list
                        return Result::ok();
                    } else {
                        UniversalFile f;

                        f.fs = get_fs();
                        f.is_dir = catalog->files[i].type == 0xFF;
                        f.is_deleted = is_deleted;
                        f.is_protected = (catalog->files[i].type & 0x80) != 0;
                        f.attributes = catalog->files[i].type & 0x7F;

                        bool updir = false;
                        if (current_path.size() > 1 && f.is_dir) {
                            const TS_PAIR parent_ts = current_path[current_path.size()-2];
                            updir = catalog->files[i].tbl_track == parent_ts.track && catalog->files[i].tbl_sector == parent_ts.sector;
                        }
                        if (updir)
                            f.name = "..";
                        else
                            f.name = trim(agat_to_utf(catalog->files[i].name, 30));

                        const auto T = attr_to_type(catalog->files[i].type);
                        f.type_preferred = agat_preferred_file_type(T);
                        f.size = catalog->files[i].size * 256;

                        f.original_name.resize(30);
                        memcpy(f.original_name.data(), &catalog->files[i].name, f.original_name.size());
                        f.type_label = std::string(agat_file_types[T]);

                        //// Getting metadata from dir_entry & ts list
                        Apple_DOS_File_Metadata metadata {};
                        // Dir entry
                        metadata.dir_entry = catalog->files[i];

                        // TS List
                        const int list_track = catalog->files[i].tbl_track;
                        const int list_sector = catalog->files[i].tbl_sector;

                        if (list_track != 0xFF) {
                            const auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                            if (!ts_list) return Result::error(ErrorCode::ReadError);

                            const void * from = &(ts_list->_not_used_03);
                            std::memcpy(metadata.tsl, from, 9);
                        } else
                            std::memset(metadata.tsl, 0, sizeof(metadata.tsl));

                        f.metadata.resize(sizeof(Apple_DOS_File_Metadata));
                        memcpy(f.metadata.data(), &metadata, sizeof(metadata));

                        f.position.push_back(catalog_ts.track);
                        f.position.push_back(catalog_ts.sector);
                        f.position.push_back(i);

                        files.push_back(f);
                    }
                } //show deleted
            }

            catalog_ts.track = catalog->next_track;
            catalog_ts.sector = catalog->next_sector;

        } while (catalog_ts.track != 0);

        return Result::ok();
    }

    std::vector<ParameterDescription> fsDOS33::file_get_metadata(const UniversalFile & fd)
    {
        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        std::vector<std::pair<std::string, std::string>> options;
        options.reserve(agat_file_types.size());
        int c=0;
        for (auto s : agat_file_types) {
            options.emplace_back(s, std::to_string(c++));
        }

        const auto T = attr_to_type(fd.attributes);
        params.push_back({"type", "{$META_TYPE}", ParamType::Enum, std::to_string(T), options});

        const auto * metadata = reinterpret_cast<const Apple_DOS_File_Metadata *>(fd.metadata.data());
        const int list_track = metadata->dir_entry.tbl_track;
        const int list_sector = metadata->dir_entry.tbl_sector;

        const auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
        if (!ts_list) return params;

        BYTES ts_custom(fd.is_dir?8:9);
        const void * from = &(ts_list->_not_used_03);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        for (int i = 0; i < ts_custom.size(); i++) {
            params.push_back({"extended_"+std::to_string(i), "{$META_EXTENDED} #"+std::to_string(i), ParamType::Byte,std::to_string(ts_custom[i])});
        }

        return params;
    }

    Result fsDOS33::rename_file(const UniversalFile & fd, const std::string & new_name)
    {
        auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
        if (!catalog) return Result::error(ErrorCode::FileRenameError);

        auto * dir_entry = &(catalog->files[fd.position[2]]);

        const BYTES name_str = utf_to_agat(new_name);
        const auto len = name_str.size();
        std::memset(dir_entry->name, 0xA0, sizeof(dir_entry->name));
        std::memcpy(dir_entry->name, name_str.data(), (len <= sizeof(dir_entry->name))?len:sizeof(dir_entry->name));

        is_changed = true;
        return Result::ok();
    }

    Result fsDOS33::file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata)
    {
        uint8_t new_type = 0;
        bool is_protected = false;
        BYTES ts_custom(fd.is_dir?8:9);

        const std::string ext_prefix = "extended_";

        for (const auto& pair : metadata) {
            const std::string& key = pair.first;
            const std::string& val = pair.second;
            // std::cout << "+ " << key << "=" << val << std::endl;
            if (key == "filename") {
                if (val != fd.name) {
                    if (!rename_file(fd, val)) return Result::error(ErrorCode::FileRenameError);
                }
            } else
            if (key == "type") {
                const uint8_t val_i = std::stoi(val);
                if (val_i == 0) new_type = 0;
                else
                if (val_i == 7) new_type = 0xFF;
                else
                    new_type = 0x01 << (val_i - 1);
            } else
            if (key == "protected") {
                is_protected = (val == "true");
            } else
            if (key.compare(0, ext_prefix.size(), ext_prefix) == 0) {
                // std::cout << "! " << ext_prefix.size() << " : " << key << std::endl;
                std::string number_part = key.substr(ext_prefix.size());
                int number_out = std::stoi(number_part);

                const uint8_t val_i = std::stoi(val);
                ts_custom[number_out] = val_i;
            }

            // std::cout << "=== " << key.compare(0, ext_prefix.size(), ext_prefix) << std::endl;
        }

        if (is_protected) new_type |= 0x80;
        auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
        if (!catalog) return Result::error(ErrorCode::FileMetadataError);

        auto * dir_entry = &(catalog->files[fd.position[2]]);

        dir_entry->type = new_type;

        const int list_track = dir_entry->tbl_track;
        const int list_sector = dir_entry->tbl_sector;

        auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
        if (!ts_list) return Result::error(ErrorCode::FileMetadataError);

        void * copy_to = &(ts_list->_not_used_03);
        std::memcpy(copy_to, ts_custom.data(), ts_custom.size());

        is_changed = true;
        return Result::ok();
    }

    Result fsDOS33::restore_file(const UniversalFile & uf)
    {
        if (!uf.is_dir) {
            // ----- File
            const auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, uf.position[0], uf.position[1]));
            if (!catalog) return Result::error(ErrorCode::FileDeleteError);
            auto * dir_entry = const_cast<Apple_DOS_File *>(&(catalog->files[uf.position[2]]));

            int list_track = dir_entry->tbl_track;
            int list_sector = dir_entry->tbl_sector;

            if (list_track != 0xFF) return Result::ok();

            const uint8_t restored_track = dir_entry->name[29];

            if (restored_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                dir_entry->tbl_track = list_track = restored_track;
                dir_entry->name[29] = 0xA0;

                do {
                    if (list_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                        const auto * ts_list = reinterpret_cast<Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                        if (!ts_list) return Result::error(ErrorCode::FileRestoreError);

                        for (int i = 0; i < VTOC->pairs_on_sector; i++){
                            const int file_track = ts_list->ts[i][0];
                            const int file_sector = ts_list->ts[i][1];
                            if (file_track == 0 && file_sector == 0) break;
                            if (!sector_occupy(0, file_track, file_sector))
                                return Result::error(ErrorCode::FileRestoreError, "Sector is not free");
                        }
                        if (!sector_occupy(0, list_track, list_sector))
                                return Result::error(ErrorCode::FileRestoreError, "Sector is not free");
                        list_track = ts_list->next_track;
                        list_sector = ts_list->next_sector;
                    } else {
                        return Result::error(ErrorCode::FileRestoreError, "Incorrect track/sector data");
                    }

                } while (list_track != 0);
                is_changed = true;
                return Result::ok();
            } else {
                return Result::error(ErrorCode::FileRestoreError);
            }
        } else {
            // ----- Directory

            auto * catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, uf.position[0], uf.position[1]));
            if (!catalog) return Result::error(ErrorCode::FileRestoreError);

            Apple_DOS_File * dir_entry = &catalog->files[uf.position[2]];

            TS_PAIR catalog_ts {};

            catalog_ts.track = dir_entry->tbl_track;
            catalog_ts.sector = dir_entry->tbl_sector;

            if (catalog_ts.track != 0xFF) return Result::ok();

            const uint8_t restored_track = dir_entry->name[29];

            if (restored_track < (image->get_tracks()*image->get_heads()) && catalog_ts.sector < image->get_sectors()) {
                dir_entry->tbl_track = catalog_ts.track = restored_track;
                dir_entry->name[29] = 0xA0;
                do {
                    catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));
                    if (!catalog) return Result::error(ErrorCode::FileRestoreError);

                    if (!sector_occupy(0, catalog_ts.track, catalog_ts.sector))
                        return Result::error(ErrorCode::FileRestoreError, "Sector is not free");
                    catalog_ts.track = catalog->next_track;
                    catalog_ts.sector = catalog->next_sector;
                } while (catalog_ts.track != 0);

                is_changed = true;
            }

        }
        return Result::ok();
    }

}
