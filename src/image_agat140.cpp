#include "loader.h"
#include "image_agat140.h"

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
    }

    int imageAgat140::check()
    {
        return FDD_LOAD_OK;
    }

    // int imageAgat140::translate_sector_logic2raw(int sector)
    // {
    //     return sector; //agat_140_logic2raw[sector];
    // }

    // int imageAgat140::translate_sector_raw2logic(int sector)
    // {
    //     return sector; //agat_140_raw2logic[sector];
    // }

}
