#include <fstream>

#include "writer.h"

namespace dsk_tools {

    Writer::Writer(const std::string & format_id, diskImage * image_to_save):
          format_id(format_id)
        , image(image_to_save)
    {}

    Writer::~Writer()
    {}

    int Writer::write(const std::string & file_name)
    {
        std::ofstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_WRITE_ERROR;
        }

        BYTES buffer;

        write(buffer);

        file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

        return FDD_WRITE_OK;

    }

}
