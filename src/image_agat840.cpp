#include "image_agat840.h"

namespace dsk_tools {
    imageAgat840::imageAgat840(Loader * loader):
        imageAgat140(loader)
    {
        format_heads = 2;
        format_tracks = 80;
        format_sectors = 21;
        format_sector_size = 256;
        expected_size = format_heads * format_tracks * format_sectors * format_sector_size;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = ISOIBM_MFM_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    uint8_t * imageAgat840::get_sector_data(int head, int track, int sector)
    {
        return diskImage::get_sector_data(track & 1, track >> 1, sector);
    }


}
