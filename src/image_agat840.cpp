#include "dsk_tools/image_agat840.h"


namespace dsk_tools {
    imageAgat840::imageAgat840(Loader * loader):
        imageAgat140(loader)
    {
        format_heads = 1;
        format_tracks = 160;
        format_sectors = 21;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    int imageAgat840::translate_sector(int sector)
    {
        return sector;
    }

}
