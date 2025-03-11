#include "disk_image.h"

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
        long offset = ((track * format_heads  + head) * format_sectors + sector) * format_sector_size;
        return reinterpret_cast<uint8_t *>(&buffer[offset]);
    }

}
