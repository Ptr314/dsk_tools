// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A top level abstract class for different writers
#pragma once


#include <string>

#include "disk_image.h"

namespace dsk_tools {

    class Writer
    {
    protected:
        std::string     format_id;
        diskImage     * image;

    public:
        Writer(const std::string & format_id, diskImage *image_to_save);
        virtual ~Writer();

        virtual int write(const std::string & file_name);
        virtual int write(BYTES & buffer) = 0;
        virtual std::string get_default_ext() = 0;
        virtual int substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) = 0;
    };

}
