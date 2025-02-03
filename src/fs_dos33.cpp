#include <iostream>

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
                file.name = trim(koi7_to_utf(catalog->files[i].name, 30));
                file.preferred_type = PREFERRED_BINARY;
                switch (catalog->files[i].type & 0x7F) {
                case 0x00:
                    file.type_str_short = "TXT";
                    file.type_str = "TEXT File";
                    file.preferred_type = PREFERRED_TEXT;
                    break;
                case 0x01:
                    file.type_str_short = "IBS";
                    file.type_str = "INTEGER BASIC File";
                    break;
                case 0x02:
                    file.type_str_short = "ABS";
                    file.type_str = "APPLESOFT BASIC File";
                    break;
                case 0x04:
                    file.type_str_short = "BIN";
                    file.type_str = "BINARY File";
                    break;
                case 0x08:
                    file.type_str_short = "S";
                    file.type_str = "S Type File";
                    break;
                case 0x10:
                    file.type_str_short = "REL";
                    file.type_str = "RELOCATABLE File";
                    break;
                case 0x20:
                    file.type_str_short = "A";
                    file.type_str = "A Type File";
                    break;
                case 0x40:
                    file.type_str_short = "A";
                    file.type_str = "B Type File";
                    break;
                default:
                    file.type_str_short = "?";
                    file.type_str = "Unknown File";
                    break;
                };
                file.size = catalog->files[i].size * 256;

                // To avoid finding the file, we use a vector to store its location
                file.location.push_back(catalog->files[i].tbl_track);
                file.location.push_back(catalog->files[i].tbl_sector);

                files->push_back(file);
            }

        } while (catalog_track != 0);

        return FDD_OP_OK;
    }

    BYTES fsDOS33::get_file(const fileData & fd)
    {
        BYTES data;

        int list_track = fd.location.at(0);
        int list_sector = fd.location.at(1);

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

}
