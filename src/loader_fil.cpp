#include <fstream>
#include <iostream>

#include "loader_fil.h"

namespace dsk_tools {
    LoaderFIL::LoaderFIL(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    int LoaderFIL::load(std::vector<uint8_t> &buffer)
    {
        uint8_t res = FDD_LOAD_OK;

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        buffer.resize(fsize);
        file.read (reinterpret_cast<char*>(buffer.data()), image_size);

        loaded = true;

        return FDD_LOAD_OK;
    }

    std::string LoaderFIL::file_info()
    {
        std::string result = "";

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}\n";
            return result;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(fsize) + " {$BYTES}\n";

        return result;

    }

}
