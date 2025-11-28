// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .DSK files
#pragma once


#include "writer.h"

namespace dsk_tools {

    class WriterRAW:public Writer
    {
    public:
        WriterRAW(const std::string & format_id, diskImage *image_to_save);
        virtual std::string get_default_ext() override;
        virtual int write(BYTES & buffer) override;
        virtual int substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}
