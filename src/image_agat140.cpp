
#include "dsk_tools/dsk_tools.h"
#include "dsk_tools/loader.h"
#include "dsk_tools/image_agat140.h"

namespace dsk_tools {
    imageAgat140::imageAgat140(Loader * loader):
          diskImage(loader)
    {
        format_heads = 1;
        format_tracks = 35;
        format_sectors = 16;
        format_sector_size = 256;
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
        VTOC = reinterpret_cast<dsk_tools::Apple_DOS_VTOC *>(get_sector_data(0, 0x11, 0));
        int sector_size = static_cast<int>(VTOC->bytes_per_sector[1])*256 + VTOC->bytes_per_sector[0];
        if (VTOC->dos_release != 3 || VTOC->sectors_on_track != 16 || sector_size != 256) {
            return FDD_OPEN_BAD_FORMAT;
        }
        // qDebug() << "VTOC";
        // qDebug() << "Catalog on track: " << VTOC->catalog_track;
        // qDebug() << "Catalog on sector: " << VTOC->catalog_sector;
        // qDebug() << "DOS release: " << VTOC->dos_release;
        // qDebug() << "Volume: " << VTOC->volume;
        // qDebug() << "Pairs on sector: " << VTOC->pairs_on_sector;
        // qDebug() << "Last track: " << VTOC->last_track;
        // qDebug() << "direction: " << VTOC->direction;
        // qDebug() << "Tracks total: " << VTOC->tracks_total;
        // qDebug() << "Sectors on track: " << VTOC->sectors_on_track;
        // qDebug() << "Bytes per sector: " << static_cast<int>(VTOC->bytes_per_sector[1])*256 + VTOC->bytes_per_sector[0];
        return FDD_OPEN_OK;
    }
}
