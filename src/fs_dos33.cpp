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
        return FILE_PROTECTION | FILE_TYPE | FILE_DELETE;
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

    int fsDOS33::dir(std::vector<dsk_tools::fileData> * files)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        TS_PAIR catalog_ts = current_path.back();

        Apple_DOS_Catalog * catalog;

        // if (current_path.size() > 1) {
        //     fileData updir;
        //     updir.name = "..";
        //     updir.is_dir = true;
        //     updir.is_deleted = false;
        //     files->push_back(updir);
        // }

        do {
            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_ts.track, catalog_ts.sector));

            for (int i=0; i<7; i++) {
                fileData file;

                if (catalog->files[i].tbl_track == 0) return FDD_OP_OK;
                file.is_dir = catalog->files[i].type == 0xFF;
                file.is_deleted = catalog->files[i].tbl_track == 0xFF;
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

                if (T == 0)
                    file.preferred_type = PREFERRED_TEXT;
                else if (T == 2)
                    file.preferred_type = PREFERRED_ABS;
                else
                    file.preferred_type = PREFERRED_BINARY;

                file.size = catalog->files[i].size * 256;

                file.metadata.resize(sizeof(catalog->files[i]));
                memcpy(file.metadata.data(), &(catalog->files[i]), sizeof(catalog->files[i]));

                files->push_back(file);
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
        BYTES ts_custom(9);
        void * from = &(ts_list->_not_used_03);
        std::memcpy(ts_custom.data(), from, 9);
        result += "    {$CUSTOM_DATA}: [" +  toHexList(ts_custom, "$") +"]\n";

        result += "\n";

        do {
            result += "T/S "  + std::to_string(list_track) + ":" + std::to_string(list_sector) + "\n";
            if (list_track < (image->get_tracks()*image->get_heads()) && image->get_sectors()) {
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
        return agat_vtoc_info(*VTOC);
    }


}
