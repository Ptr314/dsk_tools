#include <fstream>
#include <iostream>

#include "dsk_tools/dsk_tools.h"
#include "dsk_tools/loader_raw.h"

namespace dsk_tools {
    LoaderRAW::LoaderRAW(std::string file_name, std::string format_id, std::string type_id, bool msb_first):
        Loader(file_name, format_id, type_id)
        , msb_first(msb_first)
    {}

    int LoaderRAW::load(std::vector<uint8_t> * buffer)
    {
        uint8_t res = FDD_LOAD_OK;

        std::cout << "File name: " << file_name << std::endl;

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        std::cout << "File size: " << fsize << std::endl;

        buffer->resize(fsize);
        file.read (reinterpret_cast<char*>(buffer->data()), fsize);

        loaded = true;

        return FDD_LOAD_OK;
    }
}
