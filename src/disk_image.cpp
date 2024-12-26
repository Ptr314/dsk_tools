#include "dsk_tools/disk_image.h"

namespace dsk_tools {

    diskImage::diskImage(Loader * loader):
        loader(loader)
    {

    }

    int diskImage::load()
    {
        return loader->load(&buffer);
    }

    std::byte * diskImage::get_sector_data(int head, int track, int sector)
    {
        // Assumes sector numbering from 0
        long offset = ((track * format_heads  + head) * format_sectors + translate_sector(sector)) * format_sector_size;
        return reinterpret_cast<std::byte *>(&buffer[offset]);
    }
    int diskImage::translate_sector(int sector)
    {
        // Assumes sector numbering from 1
        // Assumes no interleving
        // Needs to be overridden in inheriting classes otherwise
        return sector-1;
    }

}
