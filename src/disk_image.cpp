#include "dsk_tools/disk_image.h"
#include "dsk_tools/dsk_tools.h"

namespace dsk_tools {

    diskImage::diskImage(Loader * loader):
          loader(loader)
        , is_loaded(false)
        , is_open(false)
    {}

    int diskImage::load()
    {
        type_id = loader->get_type_id();
        int result = loader->load(&buffer);
        is_loaded = (result == FDD_LOAD_OK);
        return result;
    }

    uint8_t * diskImage::get_sector_data(int head, int track, int sector)
    {
        // Assumes sector numbering from 0
        long offset = ((track * format_heads  + head) * format_sectors + translate_sector(sector)) * format_sector_size;
        return reinterpret_cast<uint8_t *>(&buffer[offset]);
    }

    int diskImage::translate_sector(int sector)
    {
        // Assumes sector numbering from 1
        // Assumes no interleving
        // Needs to be overridden in inheriting classes otherwise
        return sector-1;
    }

    std::string diskImage::file_name()
    {
        return loader->get_file_name();
    }

    int diskImage::get_heads()
    {
        return format_heads;
    }

    int diskImage::get_tracks()
    {
        return format_tracks;
    }

    int diskImage::get_sectors()
    {
        return format_sectors;
    }

    int diskImage::get_bitrate()
    {
        return format_bitrate;
    }

    int diskImage::get_rpm()
    {
        return format_rpm;
    }

    int diskImage::get_track_encoding()
    {
        return format_track_encoding;
    }

    int diskImage::get_floppyinterfacemode()
    {
        return format_floppyinterfacemode;
    }

    std::string diskImage::get_type_id()
    {
        return type_id;
    }


}
