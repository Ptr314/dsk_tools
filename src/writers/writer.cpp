// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A top level abstract class for different writers

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
