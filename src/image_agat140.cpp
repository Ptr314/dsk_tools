#include <filesystem>
#include <iostream>

#include "dsk_tools/loader.h"
#include "dsk_tools/image_agat140.h"
#include "dsk_tools/utils.h"

namespace dsk_tools {
    imageAgat140::imageAgat140(Loader * loader):
          diskImage(loader)
    {
        format_heads = 1;
        format_tracks = 35;
        format_sectors = 16;
        format_sector_size = 256;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;


        std::cout << std::filesystem::current_path() << std::endl;

        // std::ifstream charmap_file(, std::ios::in | std::ios::binary);
    }

    int imageAgat140::check()
    {
        return FDD_LOAD_OK;
    }

    int imageAgat140::translate_sector(int sector)
    {
        return agat_140_raw2dos[sector];
    }

    int imageAgat140::get_capabilities()
    {
        return FILE_PRORECTION | FILE_TYPE | FILE_DELETE;
    }

    int imageAgat140::open()
    {
        if (!is_loaded) return FDD_OPEN_NOT_LOADED;

        VTOC = reinterpret_cast<dsk_tools::Apple_DOS_VTOC *>(get_sector_data(0, 0x11, 0));
        int sector_size = static_cast<int>(VTOC->bytes_per_sector);

        std::cout << "VTOC";
        std::cout << "Catalog on track: " << (int)VTOC->catalog_track << std::endl;
        std::cout << "Catalog on sector: " << (int)VTOC->catalog_sector << std::endl;
        std::cout << "DOS release: " << (int)VTOC->dos_release << std::endl;
        std::cout << "Volume: " << (int)VTOC->volume_id << std::endl;
        std::cout << "Pairs on sector: " << (int)VTOC->pairs_on_sector << std::endl;
        std::cout << "Last track: " << (int)VTOC->last_track << std::endl;
        std::cout << "direction: " << (int)VTOC->direction << std::endl;
        std::cout << "Tracks total: " << (int)VTOC->tracks_total << std::endl;
        std::cout << "Sectors on track: " << (int)VTOC->sectors_on_track << std::endl;
        std::cout << "Bytes per sector: " << static_cast<int>(VTOC->bytes_per_sector) << std::endl;

        if (VTOC->dos_release != 3 || VTOC->sectors_on_track != format_sectors || sector_size != format_sector_size) {
            return FDD_OPEN_BAD_FORMAT;
        }

        // int catalog_track = VTOC->catalog_track;
        // int catalog_sector = VTOC->catalog_sector;

        // Apple_DOS_Catalog * catalog;

        // do {

        //     std::cout << "Catalog t/s: " << catalog_track << "/" << catalog_sector << std::endl;

        //     catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(get_sector_data(0, catalog_track, catalog_sector));

        //     catalog_track = catalog->next_track;
        //     catalog_sector = catalog->next_sector;

        //     std::cout << "Next t/s: " << catalog_track << "/" << catalog_sector << std::endl;

        //     for (int i=0; i<7; i++) {
        //         if (catalog->files[i].track == 0) break;
        //         std::cout << "File entry " << i << ": t/s=" << (int)catalog->files[i].track << "/" << (int)catalog->files[i].sector << " ";
        //         std::cout << "Type: $" << std::hex << (int)catalog->files[i].type << " ";
        //         std::cout << "Size: " << std::dec << (int)catalog->files[i].size << " ";
        //         for (int j=0; j<30; j++) std::cout << koi7map[catalog->files[i].name[j] & 0x7F];
        //         // for (int j=0; j<30; j++) std::cout << " " << std::hex << (int)catalog->files[i].name[j];
        //         std::cout << std::endl;
        //     }
        //     std::cout << std::endl;

        // } while (catalog_track != 0);

        is_open = true;

        return FDD_OPEN_OK;
    }

    int imageAgat140::dir(std::vector<dsk_tools::fileData> * files)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        int catalog_track = VTOC->catalog_track;
        int catalog_sector = VTOC->catalog_sector;

        Apple_DOS_Catalog * catalog;


        do {
            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(get_sector_data(0, catalog_track, catalog_sector));

            catalog_track = catalog->next_track;
            catalog_sector = catalog->next_sector;

            for (int i=0; i<7; i++) {
                fileData file;

                if (catalog->files[i].tbl_track == 0) break;
                file.is_deleted = (catalog->files[i].type == 0xFF);
                file.is_protected = (catalog->files[i].type & 0x80) != 0;
                file.attributes = catalog->files[i].type & 0x7F;
                memcpy(&file.original_name, &catalog->files[i].name, 30);
                file.original_name_length = 30;
                file.name = trim(koi7_to_utf(catalog->files[i].name, 30));
                switch (catalog->files[i].type & 0x7F) {
                    case 0x00:
                        file.type_str_short = "TXT";
                        file.type_str = "TEST File";
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

    std::vector<uint8_t> imageAgat140::get_file(const fileData & fd)
    {
        std::vector<uint8_t> data;

        int list_track = fd.location.at(0);
        int list_sector = fd.location.at(1);

        Apple_DOS_TS_List * ts_list;

        do {
            ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(get_sector_data(0, list_track, list_sector));

            for (int i = 0; i < VTOC->pairs_on_sector; i++){
                int file_track = ts_list->ts[i][0];
                int file_sector = ts_list->ts[i][1];
                if (file_track == 0) break;
                std::uint8_t * sector = get_sector_data(0, file_track, file_sector);
                data.insert(data.end(),&sector[0],&sector[256]);
            }

            list_track = ts_list->next_track;
            list_sector = ts_list->next_sector;

        } while (list_track != 0);

        return data;
    }
}
