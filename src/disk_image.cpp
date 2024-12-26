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
        int result = loader->load(&buffer);
        is_loaded = (result == FDD_LOAD_OK);
        return result;
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
