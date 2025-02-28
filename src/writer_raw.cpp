#include <iostream>

#include "writer_raw.h"

namespace dsk_tools {

    WriterRAW::WriterRAW(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    std::string WriterRAW::get_default_ext()
    {
        return "dsk";
    }


    int WriterRAW::write(BYTES &buffer)
    {
        buffer = *image->get_buffer();
        return FDD_WRITE_OK;
    }

    int WriterRAW::substitute_tracks(BYTES & buffer, BYTES &tmplt, const int numtracks)
    {
        if (buffer.size() != tmplt.size()) return FDD_WRITE_INCORECT_TEMPLATE;
        if (buffer.size() != image->get_size()) return FDD_WRITE_INCORECT_SOURCE;
        int block_size = image->get_sectors() * image->get_sector_size() * image->get_heads();

        BYTES out;
        out.insert(out.end(), tmplt.begin(), tmplt.begin() + block_size);
        out.insert(out.end(), buffer.begin() + block_size, buffer.end());

        buffer = out;
        return FDD_WRITE_OK;
    }
}
