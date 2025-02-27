#include "dsk_tools/dsk_tools.h"
#include <iostream>

namespace dsk_tools {

diskImage::diskImage(Loader * loader):
          loader(loader)
        , is_loaded(false)
    {}

    diskImage::~diskImage()
    {
        if (loader) delete loader;
    }

    int diskImage::load()
    {
        type_id = loader->get_type_id();
        int result = loader->load(buffer);
        if (result == FDD_LOAD_OK) {
            int buffer_size = buffer.size();
            if (buffer_size >= expected_size && buffer_size <= expected_size + 4) {
                is_loaded = true;
                return FDD_LOAD_OK;
            } else {
                return FDD_LOAD_SIZE_MISMATCH;
            }
        }
        return result;
    }

    uint8_t * diskImage::get_sector_data(int head, int track, int sector)
    {
        // return get_raw_sector_data(head, track, translate_sector_logic2raw(sector));
        long offset = ((track * format_heads  + head) * format_sectors + sector) * format_sector_size;
        return reinterpret_cast<uint8_t *>(&buffer[offset]);
    }

    // uint8_t * diskImage::get_raw_sector_data(int head, int track, int sector)
    // {
    //     // Assumes sector numbering from 0
    //     long offset = ((track * format_heads  + head) * format_sectors + sector) * format_sector_size;
    //     return reinterpret_cast<uint8_t *>(&buffer[offset]);
    // }

    // int diskImage::translate_sector_logic2raw(int sector)
    // {
    //     // Assumes logic sector numbering from 1
    //     // Assumes no interleving
    //     // Needs to be overridden in inheriting classes otherwise
    //     return sector-1;
    // }

    // int diskImage::translate_sector_raw2logic(int sector)
    // {
    //     // Assumes logic sector numbering from 1
    //     // Assumes no interleving
    //     // Needs to be overridden in inheriting classes otherwise
    //     return sector+1;
    // }

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

    BYTES * diskImage::get_buffer()
    {
        return &buffer;
    }

    bool diskImage::get_loaded()
    {
        return is_loaded;
    }

}
