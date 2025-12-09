// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - loader part

#include <fstream>
#include <iostream>

#include "host_helpers.h"

#include "loader_fil.h"

namespace dsk_tools {
    LoaderFIL::LoaderFIL(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    Result LoaderFIL::load(std::vector<uint8_t> &buffer)
    {
        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return Result::error(ErrorCode::LoadError, "Cannot open file");
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

        image_size = fsize;

        buffer.resize(image_size);
        file.read (reinterpret_cast<char*>(buffer.data()), image_size);

        loaded = true;

        return Result::ok();
    }

    std::string LoaderFIL::file_info()
    {
        std::string result = "";

        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}\n";
            return result;
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(fsize) + " {$BYTES}\n";

        return result;

    }

}
