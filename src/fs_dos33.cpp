#include <iostream>
#include <fstream>
#include <cstring>

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

        VTOC = reinterpret_cast<dsk_tools::Apple_DOS_VTOC *>(image->get_sector_data(0, 0x11, 0));
        int sector_size = static_cast<int>(VTOC->bytes_per_sector);

        if (VTOC->dos_release != 3 || VTOC->sectors_on_track != image->get_sectors() || sector_size != 256) {
            return FDD_OPEN_BAD_FORMAT;
        }

        is_open = true;
        return FDD_OPEN_OK;
    }

    std::string fsDOS33::attr_to_type(uint8_t a)
    {
        // http://forum.agatcomp.ru//viewtopic.php?id=193
        std::vector<std::string> types = {"T", "I", "A", "B", "S", "П", "К", "Д"};

        uint8_t v = a & 0x7F;
        int n = 0;
        do {
            if (v == 0 ) return types[n];
            v <<= 1;
            n++;
        } while (n < 9);
        return "";
    }

    int fsDOS33::dir(std::vector<dsk_tools::fileData> * files)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        int catalog_track = VTOC->catalog_track;
        int catalog_sector = VTOC->catalog_sector;

        Apple_DOS_Catalog * catalog;


        do {
            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, catalog_track, catalog_sector));

            catalog_track = catalog->next_track;
            catalog_sector = catalog->next_sector;

            for (int i=0; i<7; i++) {
                fileData file;

                if (catalog->files[i].tbl_track == 0) break;
                file.is_dir = false;
                file.is_deleted = (catalog->files[i].type == 0xFF);
                file.is_protected = (catalog->files[i].type & 0x80) != 0;
                file.attributes = catalog->files[i].type & 0x7F;
                memcpy(&file.original_name, &catalog->files[i].name, 30);
                file.original_name_length = 30;
                file.name = trim(agat_to_utf(catalog->files[i].name, 30));
                file.type_str_short = attr_to_type(catalog->files[i].type);
                if (file.type_str_short == "T")
                    file.preferred_type = PREFERRED_TEXT;
                else
                    file.preferred_type = PREFERRED_BINARY;

                file.size = catalog->files[i].size * 256;

                file.metadata.resize(sizeof(catalog->files[i]));
                memcpy(file.metadata.data(), &(catalog->files[i]), sizeof(catalog->files[i]));


                files->push_back(file);
            }

        } while (catalog_track != 0);

        return FDD_OP_OK;
    }

    BYTES fsDOS33::get_file(const fileData & fd)
    {
        BYTES data;

        const dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<const dsk_tools::Apple_DOS_File *>(fd.metadata.data());
        int list_track = dir_entry->tbl_track;
        int list_sector = dir_entry->tbl_sector;

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

    void fsDOS33::cd(const dsk_tools::fileData & dir)
    {}

    std::string fsDOS33::file_info(const fileData & fd) {
        std::string result = "";
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
            //header.tsl
        }

        return FDD_WRITE_OK;
    }

}
