#include <fstream>


#include "writer_raw.h"

namespace dsk_tools {

    WriterRAW::WriterRAW(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    std::string WriterRAW::get_default_ext()
    {
        return "dsk";
    }


    int WriterRAW::write(const std::string & file_name)
    {
        std::ofstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_WRITE_ERROR;
        }

        BYTES * buffer = image->get_buffer();

        file.write(reinterpret_cast<char*>(buffer->data()), buffer->size());

        file.close();

        return 0;
    }
}
