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

    int fsDOS33::get_capabilities()
    {
        return FILE_PROTECTION | FILE_TYPE | FILE_DELETE | FILE_ADD | FILE_DIRECTORIES | FILE_RENAME;
    }

    FSCaps fsDOS33::getCaps()
    {
        return FSCaps::Protect | FSCaps::Types | FSCaps::Delete | FSCaps::Add | FSCaps::Dirs | FSCaps::Rename;
    }

    int fsDOS33::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        VTOC = reinterpret_cast<dsk_tools::Agat_VTOC *>(image->get_sector_data(0, 0x11, 0));
        int sector_size = static_cast<int>(VTOC->bytes_per_sector);

        // Also: https://retrocomputing.stackexchange.com/questions/15054/how-can-i-programmatically-determine-whether-an-apple-ii-dsk-disk-image-is-a-do
        if (VTOC->sectors_on_track != image->get_sectors() || sector_size != 256) {
            return FDD_OPEN_BAD_FORMAT;
        }

        TS_PAIR root_ts = {
            VTOC->catalog_track,
            VTOC->catalog_sector
        };

        current_path.push_back(root_ts);

        is_open = true;
        volume_id = VTOC->volume_id;
        return FDD_OPEN_OK;
    }

    int fsDOS33::attr_to_type(uint8_t a)
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

    int fsDOS33::dir(std::vector<dsk_tools::fileData> * files, bool show_deleted)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        TS_PAIR catalog_ts = current_path.back();

        // std::cout << "DIR: " << (int)catalog_ts.track << ":" << (int)catalog_ts.sector << std::endl;

        Apple_DOS_Catalog * catalog;

        do {
            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));

            // std::cout << "CATALOG: " << (int)catalog_ts.track << ":" << (int)catalog_ts.sector << std::endl;

            for (int i=0; i<7; i++) {
                bool is_deleted = catalog->files[i].tbl_track == 0xFF;
                if (!is_deleted || show_deleted) {
                    fileData file;

                    if (catalog->files[i].tbl_track == 0) {
                        // Means end of the list
                        return FDD_OP_OK;
                    } else {
                        file.is_dir = catalog->files[i].type == 0xFF;
                        file.is_deleted = is_deleted;
                        file.is_protected = (catalog->files[i].type & 0x80) != 0;
                        file.attributes = catalog->files[i].type & 0x7F;
                        memcpy(&file.original_name, &catalog->files[i].name, 30);
                        file.original_name_length = 30;

                        bool updir = false;
                        if (current_path.size() > 1 && file.is_dir) {
                            TS_PAIR parent_ts = current_path[current_path.size()-2];
                            updir = catalog->files[i].tbl_track == parent_ts.track && catalog->files[i].tbl_sector == parent_ts.sector;
                        }
                        if (updir)
                            file.name = "..";
                        else
                            file.name = trim(agat_to_utf(catalog->files[i].name, 30));

                        auto T = attr_to_type(catalog->files[i].type);
                        file.type_str_short = std::string(agat_file_types[T]);
                        file.preferred_type = agat_preferred_file_type(T);
                        file.size = catalog->files[i].size * 256;

                        file.metadata.resize(sizeof(catalog->files[i]));
                        memcpy(file.metadata.data(), &(catalog->files[i]), sizeof(catalog->files[i]));

                        file.position.push_back(catalog_ts.track);
                        file.position.push_back(catalog_ts.sector);
                        file.position.push_back(i);

                        files->push_back(file);
                    }
                } //show deleted
            }

            catalog_ts.track = catalog->next_track;
            catalog_ts.sector = catalog->next_sector;

        } while (catalog_ts.track != 0);

        return FDD_OP_OK;
    }

    BYTES fsDOS33::get_file(const fileData & fd)
    {
        BYTES data;

        const dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<const dsk_tools::Apple_DOS_File *>(fd.metadata.data());
        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

        if (list_track == 0xFF) {
            list_track = dir_entry->name[29];
        }

        Apple_DOS_TS_List * ts_list;

        do {
            ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));

            for (int i = 0; i < VTOC->pairs_on_sector; i++){
                int file_track = ts_list->ts[i][0];
                int file_sector = ts_list->ts[i][1];
                if (file_track == 0) break;
                std::uint8_t * sector = image->get_sector_data(0, file_track, file_sector);
                data.insert(data.end(),&sector[0],&sector[256]);
            }

            list_track = ts_list->next_track;
            list_sector = ts_list->next_sector;

        } while (list_track != 0);

        return data;
    }

    void fsDOS33::cd_up()
    {
        if (current_path.size() > 1) current_path.pop_back();
    }

    void fsDOS33::cd(const dsk_tools::fileData & dir)
    {
        if (dir.name == "..") {
            cd_up();
        } else {
            Apple_DOS_File f;
            memcpy(&f, dir.metadata.data(), sizeof(f));

            // std::cout << "CD: " << (int)f.tbl_track << ":" << (int)f.tbl_sector << std::endl;

            current_path.push_back(
                {
                    f.tbl_track,
                    f.tbl_sector
                }
            );
        }
    }

    std::string fsDOS33::file_info(const fileData & fd) {
        const dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<const dsk_tools::Apple_DOS_File *>(fd.metadata.data());

        std::string result = "";
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

        ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
        BYTES ts_custom(fd.is_dir?8:9);
        void * from = &(ts_list->_not_used_03);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        result += "    {$CUSTOM_DATA}: [" +  toHexList(ts_custom, "$") +"]\n";

        result += "\n";

        BYTES buffer = get_file(fd);
        result += agat_vr_info(buffer);

        result += "{$TS_LIST_DATA}:\n";
        do {
            result += "T/S "  + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
            if (list_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));


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

    int fsDOS33::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
    {
        const Apple_DOS_File * dir_entry = reinterpret_cast<const Apple_DOS_File *>(fd.metadata.data());

        BYTES buffer = get_file(fd);
        if (buffer.size() > 0) {
            if (format_id == "FILE_BINARY") {
                std::ofstream file(file_name, std::ios::binary);

                if (!file.good()) {
                    return FDD_WRITE_ERROR;
                }

                file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            } else
            if (format_id == "FILE_FIL") {
                FIL_header header;
                memcpy(&header.name, &(dir_entry->name), sizeof(header.name));
                header.type = dir_entry->type;
                if (attr_to_type(dir_entry->type) == 6) {
                    // K Type
                    // http://agatcomp.ru/agat/PCutils/FileType/FIL.shtml
                    int list_track = dir_entry->tbl_track;
                    int list_sector = dir_entry->tbl_sector;

                    if (list_track != 0xFF) {
                        Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                        void * from = &(ts_list->_not_used_03);
                        std::memcpy(header.tsl, from, 9);
                    } else
                        std::memset(header.tsl, 0, sizeof(header.tsl));
                } else
                    std::memset(header.tsl, 0, sizeof(header.tsl));

                std::ofstream file(file_name, std::ios::binary);

                file.write(reinterpret_cast<char*>(&header), sizeof(FIL_header));
                file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            } else
                return FDD_WRITE_UNSUPPORTED;

            return FDD_WRITE_OK;
        } else {
            return FDD_WRITE_ERROR_READING;
        }
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

    uint32_t * fsDOS33::track_map(int track)
    {
        Agat_VTOC_Ex * VTOCEx;
        int tracks_count = image->get_tracks()*image->get_heads();
        if (track < 0x32)
            return &(VTOC->free_sectors[track]);
        else
        if (track < 0x72 && tracks_count > 0x32) {
            VTOCEx = reinterpret_cast<dsk_tools::Agat_VTOC_Ex *>(image->get_sector_data(0, 0x32, 0));
            return &(VTOCEx->free_sectors[track-0x32]);
        } else
        if (track < 0xB2 && tracks_count > 0x72) {
            VTOCEx = reinterpret_cast<dsk_tools::Agat_VTOC_Ex *>(image->get_sector_data(0, 0x72, 0));
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
        *track_map(track) |= (image->get_sectors()==16)?VTOCMask140[sector]:VTOCMask840[sector];
        // std::cout << "FREE " << track << " " << sector << std::endl;
    }

    bool fsDOS33::sector_occupy(int head, int track, int sector)
    {
        if (!sector_is_free(0, track, sector)) return false;
        *track_map(track) &= ~((image->get_sectors()==16)?VTOCMask140[sector]:VTOCMask840[sector]);
        return true;
    }

    int fsDOS33::file_delete(const fileData & fd)
    {
        if (!fd.is_dir) {
            // ----- File
            Apple_DOS_Catalog * catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
            dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<dsk_tools::Apple_DOS_File *>(&(catalog->files[fd.position[2]]));

            int list_track = dir_entry->tbl_track;
            int list_sector = dir_entry->tbl_sector;

            if (list_track == 0xFF) return FILE_DELETE_OK;

            Apple_DOS_TS_List * ts_list;

            do {
                if (list_track < (image->get_tracks()*image->get_heads()) && list_sector < image->get_sectors()) {
                    ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
                    for (int i = 0; i < VTOC->pairs_on_sector; i++){
                        int file_track = ts_list->ts[i][0];
                        int file_sector = ts_list->ts[i][1];
                        if (file_track == 0 && file_sector == 0) break;
                        sector_free(0, file_track, file_sector);
                    }
                    sector_free(0, list_track, list_sector);
                    list_track = ts_list->next_track;
                    list_sector = ts_list->next_sector;
                } else {
                    return false;
                }

            } while (list_track != 0);

            dir_entry->name[29] = dir_entry->tbl_track;
            dir_entry->tbl_track = 0xFF;

            is_changed = true;

            return FILE_DELETE_OK;
        } else {
            // ----- Directory

            // Checking if it is empty
            Apple_DOS_Catalog * catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
            dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<dsk_tools::Apple_DOS_File *>(&(catalog->files[fd.position[2]]));

            TS_PAIR catalog_ts, parent_ts;

            catalog_ts.track = dir_entry->tbl_track;
            catalog_ts.sector = dir_entry->tbl_sector;

            parent_ts = current_path[current_path.size()-2];

            int files_count = 0;
            bool first_part = true;

            do {
                catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));

                for (int i=0; i<7; i++) {
                    uint8_t t = catalog->files[i].tbl_track;
                    bool updir = (first_part && i==0 && catalog->files[i].type==0xFF);
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
                    sector_free(0, catalog_ts.track, catalog_ts.sector);
                    catalog_ts.track = catalog->next_track;
                    catalog_ts.sector = catalog->next_sector;
                } while (catalog_ts.track != 0);

                dir_entry->name[29] = dir_entry->tbl_track;
                dir_entry->tbl_track = 0xFF;
                is_changed = true;
            } else
                return FDD_DIR_NOT_EMPTY;

            return FILE_DELETE_OK;
        }
    }

    bool fsDOS33::find_empty_sector(uint8_t start_track, TS_PAIR &ts, bool go_forward)
    {
        // std::cout << "?? " << (int)start_track << " ";

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

                // std::cout << "! " << track << ":" << sector << std::endl;

                return true;
            } else {
                track += (track >= start_track)?1:-1;
                if (track < 0 || track >= image->get_tracks())
                    return false;
            }
        } while (true);
    }

    bool fsDOS33::find_epmty_dir_entry(Apple_DOS_File *& dir_entry, bool just_check, bool & extra_sector)
    {

        TS_PAIR catalog_ts = current_path.back();
        Apple_DOS_Catalog * catalog;
        TS_PAIR last_ts;

        do {
            catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));

            for (int i=0; i<7; i++) {
                uint8_t tbl_track = catalog->files[i].tbl_track;
                if (tbl_track == 0xFF || tbl_track == 0x00) {
                    dir_entry = &(catalog->files[i]);
                    extra_sector = false;
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
            TS_PAIR new_ts;
            bool res = find_empty_sector(last_ts.track, new_ts, true);
            if (res) {
                sector_occupy(0, new_ts.track, new_ts.sector);
                catalog->next_track = new_ts.track;
                catalog->next_sector = new_ts.sector;

                Apple_DOS_Catalog * new_catalog = reinterpret_cast<Apple_DOS_Catalog *>(image->get_sector_data(0, new_ts.track, new_ts.sector));
                std::memset(new_catalog, 0, sizeof(Apple_DOS_Catalog));
                dir_entry = &(new_catalog->files[0]);
                return true;
            } else
                return false;
        }
        return true;
    }

    int fsDOS33::file_add(const std::string & file_name, const std::string & format_id)
    {
        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FILE_ADD_ERROR_IO;
        }

        file.seekg (0, file.end);
        int fsize = file.tellg();
        file.seekg (0, file.beg);

        BYTES buffer;

        uint8_t type;
        uint8_t name[30];
        uint8_t tsl[9];

        // --------------- Preparing metadata
        int body_size;
        if (format_id == "FILE_FIL") {
            FIL_header header;
            file.read (reinterpret_cast<char*>(&header), sizeof(FIL_header));
            type = header.type;
            std::memcpy(name, header.name, sizeof(name));
            std::memcpy(tsl, header.tsl, 9);
            body_size = fsize - sizeof(FIL_header);
        } else {
            type = 0;

            std::memset(name, 0xA0, sizeof(name));
            BYTES name_str = utf_to_agat(get_filename(file_name));
            int len = name_str.size();
            std::memcpy(name, name_str.data(), (len <= sizeof(name))?len:sizeof(name));

            std::memset(tsl, 0, sizeof(tsl));

            body_size = fsize;
        }

        buffer.resize(body_size);
        file.read (reinterpret_cast<char*>(buffer.data()), body_size);


        // --------------- Checking for sufficient space

        int sectors_body = (body_size+255)/256;                                                   // File body
        int ts_pairs = sectors_body + 1;                                                          // We need an extra 0:0 pair to finish the list;
        int ts_lists = (ts_pairs + VTOC->pairs_on_sector - 1) / VTOC->pairs_on_sector;    // T/S lists, 122 sectors each.

        // Check if we need a new sector for catalog
        Apple_DOS_File * dir_entry;
        bool extra_sector;
        if (!find_epmty_dir_entry(dir_entry, true, extra_sector))
            return FILE_ADD_ERROR;
        int sectors_catalog = (extra_sector)?1:0;

        int sectors_total = sectors_body + ts_lists + sectors_catalog;

        if (sectors_total > free_sectors())
            return FILE_ADD_ERROR_SPACE;


        // ----------------   Main process

        // Create a directory entry
        if (!find_epmty_dir_entry(dir_entry, false, extra_sector))
            return FILE_ADD_ERROR;

        std::memset(dir_entry, 0, sizeof(Apple_DOS_File));
        dir_entry->type = type;
        dir_entry->size = sectors_body + ts_lists;
        std::memcpy(dir_entry->name, name, sizeof(name));

        Apple_DOS_TS_List * last_ts_list;

        // Filling T/S lists
        for (int i=0; i < ts_lists; i++) {

            // std::cout << "TS List: " << i << std::endl;

            TS_PAIR catalog_ts = current_path.back();
            TS_PAIR ts;
            bool res = find_empty_sector(catalog_ts.track, ts, false);
            if (!res)
                return FILE_ADD_ERROR;

            // std::cout << ">" << (int)ts.track << ":" << (int)ts.sector << std::endl;

            sector_occupy(0, ts.track, ts.sector);
            Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, ts.track, ts.sector));
            std::memset(ts_list, 0, sizeof(Apple_DOS_TS_List));
            if (i==0) {
                // Store first T/S to a catalog entry
                // std::cout << "First, store to dir" << std::endl;

                dir_entry->tbl_track = ts.track;
                dir_entry->tbl_sector = ts.sector;

                // Set TSL for the first T/S list
                void * to_ptr = &(ts_list->_not_used_03);
                std::memcpy(to_ptr, tsl, 9);

            } else {
                // Secondary T/S lists make a chain
                // std::cout << "Subsequent, store to previous" << std::endl;
                ts_list->offset = i * VTOC->pairs_on_sector;
                last_ts_list->next_track = ts.track;
                last_ts_list->next_sector = ts.sector;
            }
            uint8_t start_track = ts.track;
            for (int j=0; j < VTOC->pairs_on_sector; j++) {
                int ts_pair = i*VTOC->pairs_on_sector + j;
                if (ts_pair < ts_pairs-1) {
                    // File part
                    TS_PAIR file_ts;
                    res = find_empty_sector(start_track, file_ts, false);
                    if (!res)
                        return FILE_ADD_ERROR;

                    // std::cout << (int)file_ts.track << ":" << (int)file_ts.sector << std::endl;

                    sector_occupy(0, file_ts.track, file_ts.sector);
                    ts_list->ts[j][0] = file_ts.track;
                    ts_list->ts[j][1] = file_ts.sector;

                    uint8_t * data = image->get_sector_data(0, file_ts.track, file_ts.sector);
                    std::memcpy(data, buffer.data() + ts_pair * image->get_sector_size(), image->get_sector_size());

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
        return FILE_ADD_OK;
    }

    int fsDOS33::mkdir(const std::string & dir_name)
    {
        int sectors_body = 1;  // We need at least 1 sector for the new directory

        // Check if we need a new sector for catalog
        Apple_DOS_File * dir_entry;
        bool extra_sector;
        if (!find_epmty_dir_entry(dir_entry, true, extra_sector))
            return FILE_ADD_ERROR;
        int sectors_catalog = (extra_sector)?1:0;

        int sectors_total = sectors_body + sectors_catalog;

        if (sectors_total > free_sectors())
            return FDD_DIR_ERROR_SPACE;

        // Create a directory entry
        if (!find_epmty_dir_entry(dir_entry, false, extra_sector))
            return FILE_ADD_ERROR;

        // Meatdata
        std::memset(dir_entry, 0, sizeof(Apple_DOS_File));
        dir_entry->type = 0xFF;
        dir_entry->size = sectors_total;

        // Name
        std::memset(dir_entry->name, 0xA0, sizeof(dir_entry->name));
        BYTES name_str = utf_to_agat(get_filename(dir_name));
        int len = name_str.size();
        std::memcpy(dir_entry->name, name_str.data(), (len <= sizeof(dir_entry->name))?len:sizeof(dir_entry->name));

        // Catalog body
        TS_PAIR catalog_ts = current_path.back();
        TS_PAIR ts;
        bool res = find_empty_sector(catalog_ts.track, ts, false);
        if (!res)
            return FDD_DIR_ERROR;

        // std::cout << "NEW CATALOG: " << (int)ts.track << ":" << (int)ts.sector << std::endl;

        sector_occupy(0, ts.track, ts.sector);

        Apple_DOS_Catalog * new_catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, ts.track, ts.sector));
        std::memset(new_catalog, 0, sizeof(Apple_DOS_Catalog));

        dir_entry->tbl_track = ts.track;
        dir_entry->tbl_sector = ts.sector;

        // Reverse link to the parent
        std::memset(&(new_catalog->files[0]), 0, sizeof(Apple_DOS_File));
        new_catalog->files[0].tbl_track = catalog_ts.track;
        new_catalog->files[0].tbl_sector = catalog_ts.sector;
        new_catalog->files[0].type = 0xFF;
        std::memset(new_catalog->files[0].name, 0xA0, sizeof(dir_entry->name));
        std::memcpy(new_catalog->files[0].name, name_str.data(), (len <= sizeof(dir_entry->name))?len:sizeof(dir_entry->name));

        is_changed = true;
        return FDD_DIR_OK;
    }

    int fsDOS33::file_rename(const fileData & fd, const std::string & new_name)
    {
        Apple_DOS_Catalog * catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
        dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<dsk_tools::Apple_DOS_File *>(&(catalog->files[fd.position[2]]));

        BYTES name_str = utf_to_agat(new_name);
        int len = name_str.size();
        std::memset(dir_entry->name, 0xA0, sizeof(dir_entry->name));
        std::memcpy(dir_entry->name, name_str.data(), (len <= sizeof(dir_entry->name))?len:sizeof(dir_entry->name));

        is_changed = true;
        return FILE_RENAME_OK;
    }

    bool fsDOS33::is_root()
    {
        return current_path.size() == 1;
    }

    std::vector<ParameterDescription> fsDOS33::file_get_metadata(const fileData &fd)
    {
        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        std::vector<std::pair<std::string, std::string>> options;
        for (int i = 0; i < agat_file_types.size(); i++) {
            options.emplace_back(agat_file_types[i], std::to_string(i));
        }

        auto T = attr_to_type(fd.attributes);
        params.push_back({"type", "{$META_TYPE}", ParamType::Enum, std::to_string(T), options});


        const dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<const dsk_tools::Apple_DOS_File *>(fd.metadata.data());
        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

        Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
        BYTES ts_custom(fd.is_dir?8:9);
        void * from = &(ts_list->_not_used_03);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        for (int i = 0; i < ts_custom.size(); i++) {
            params.push_back({"extended_"+std::to_string(i), "{$META_EXTENDED} #"+std::to_string(i), ParamType::Byte,std::to_string(ts_custom[i])});
        }

        return params;
    }

    int fsDOS33::file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata)
    {
        uint8_t new_type = 0;
        bool is_protected = false;
        BYTES ts_custom(fd.is_dir?8:9);

        const std::string ext_prefix = "extended_";

        for (const auto& pair : metadata) {
            const std::string& key = pair.first;
            const std::string& val = pair.second;
            std::cout << "+ " << key << "=" << val << std::endl;
            if (key == "filename") {
                if (val != fd.name) {
                    int res = file_rename(fd, val);
                    if (res != FILE_RENAME_OK)
                        return FILE_METADATA_ERROR;
                }
            } else
            if (key == "type") {
                uint8_t val_i = std::stoi(val);
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

                uint8_t val_i = std::stoi(val);
                ts_custom[number_out] = val_i;
            }

            // std::cout << "=== " << key.compare(0, ext_prefix.size(), ext_prefix) << std::endl;
        }

        if (is_protected) new_type |= 0x80;
        Apple_DOS_Catalog * catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
        dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<dsk_tools::Apple_DOS_File *>(&(catalog->files[fd.position[2]]));

        dir_entry->type = new_type;

        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

        Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));

        void * copy_to = &(ts_list->_not_used_03);
        std::memcpy(copy_to, ts_custom.data(), ts_custom.size());

        is_changed = true;
        return FILE_METADATA_OK;
    }

    bool fsDOS33::file_find(const std::string & file_name, fileData & fd)
    {
        std::vector<fileData> files;
        dir(&files);
        for (const dsk_tools::fileData& f : files) {
            if (to_upper(f.name) == file_name) {
                fd = f;
                return true;
            }
        }
        return false;
    }

}
