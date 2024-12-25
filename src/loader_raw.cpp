#include <fstream>
#include <iostream>
#include <iterator>

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

        const auto begin = file.tellg();
        file.seekg (0, std::ios::end);
        const auto end = file.tellg();
        const auto fsize = (end-begin);

        std::cout << "File size: " << fsize << std::endl;

        buffer->reserve(fsize);

        buffer->insert(buffer->begin(),
                   std::istream_iterator<uint8_t>(file),
                   std::istream_iterator<uint8_t>());

        loaded = true;

        return FDD_LOAD_OK;
    }
}
