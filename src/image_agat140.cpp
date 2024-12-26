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

    int imageAgat140::open()
    {
        if (!is_loaded) return FDD_OPEN_NOT_LOADED;

        VTOC = reinterpret_cast<dsk_tools::Apple_DOS_VTOC *>(get_sector_data(0, 0x11, 0));
        int sector_size = static_cast<int>(VTOC->bytes_per_sector);
        if (VTOC->dos_release != 3 || VTOC->sectors_on_track != 16 || sector_size != 256) {
            return FDD_OPEN_BAD_FORMAT;
        }
        // std::cout << "VTOC";
        // std::cout << "Catalog on track: " << (int)VTOC->catalog_track << std::endl;
        // std::cout << "Catalog on sector: " << (int)VTOC->catalog_sector << std::endl;
        // std::cout << "DOS release: " << (int)VTOC->dos_release << std::endl;
        // std::cout << "Volume: " << (int)VTOC->volume_id << std::endl;
        // std::cout << "Pairs on sector: " << (int)VTOC->pairs_on_sector << std::endl;
        // std::cout << "Last track: " << (int)VTOC->last_track << std::endl;
        // std::cout << "direction: " << (int)VTOC->direction << std::endl;
        // std::cout << "Tracks total: " << (int)VTOC->tracks_total << std::endl;
        // std::cout << "Sectors on track: " << (int)VTOC->sectors_on_track << std::endl;
        // std::cout << "Bytes per sector: " << static_cast<int>(VTOC->bytes_per_sector) << std::endl;

        int catalog_track = VTOC->catalog_track;
        int catalog_sector = VTOC->catalog_sector;

        Apple_DOS_Catalog * catalog;

        do {

            std::cout << "Catalog t/s: " << catalog_track << "/" << catalog_sector << std::endl;

            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(get_sector_data(0, catalog_track, catalog_sector));

            catalog_track = catalog->next_track;
            catalog_sector = catalog->next_sector;

            std::cout << "Next t/s: " << catalog_track << "/" << catalog_sector << std::endl;

            for (int i=0; i<7; i++) {
                if (catalog->files[i].track == 0) break;
                std::cout << "File entry " << i << ": t/s=" << (int)catalog->files[i].track << "/" << (int)catalog->files[i].sector << " ";
                std::cout << "Type: $" << std::hex << (int)catalog->files[i].type << " ";
                std::cout << "Size: " << std::dec << (int)catalog->files[i].size << " ";
                for (int j=0; j<30; j++) std::cout << koi7map[catalog->files[i].name[j] & 0x7F];
                // for (int j=0; j<30; j++) std::cout << " " << std::hex << (int)catalog->files[i].name[j];
                std::cout << std::endl;
            }
            std::cout << std::endl;

        } while (catalog_track != 0);

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

        fileData file;

        do {
            catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(get_sector_data(0, catalog_track, catalog_sector));

            catalog_track = catalog->next_track;
            catalog_sector = catalog->next_sector;

            for (int i=0; i<7; i++) {
                if (catalog->files[i].track == 0) break;
                file.is_deleted = (catalog->files[i].type == 0xFF);
                file.is_protected = (catalog->files[i].type & 0x80) != 0;
                file.attributes = catalog->files[i].type & 0x7F;
                memcpy(&file.original_name, &catalog->files[i].name, 30);
                file.original_name_length = 30;
                file.name = koi7_to_utf(catalog->files[i].name, 30);

                files->push_back(file);
            }

        } while (catalog_track != 0);

        return FDD_OP_OK;
    }
}
