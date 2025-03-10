#include <fstream>
#include <iostream>

#include "loader_raw.h"

namespace dsk_tools {
    LoaderRAW::LoaderRAW(std::string file_name, std::string format_id, std::string type_id, bool msb_first):
        Loader(file_name, format_id, type_id)
        , msb_first(msb_first)
    {}

    int LoaderRAW::load(std::vector<uint8_t> &buffer)
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
        file.read (reinterpret_cast<char*>(buffer.data()), fsize);

        loaded = true;

        return FDD_LOAD_OK;
    }

    std::string LoaderRAW::file_info()
    {
        std::string result = "";

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}:\n";
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
