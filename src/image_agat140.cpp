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
        expected_size = format_heads * format_tracks * format_sectors * format_sector_size;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    int imageAgat140::check()
    {
        // TODO: Implement
        return FDD_LOAD_OK;
    }

}
