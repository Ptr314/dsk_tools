// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .DSK files
#pragma once


#include "writer.h"

namespace dsk_tools {

    class WriterRAW:public Writer
    {
    protected:
        std::string file_name;

    public:
        // file_name is where the image goes: a 140 Kb image goes into a .po in ProDOS
        // order, into any other file in DOS 3.3 order, and with none, back into the order
        // of the file it came from
        WriterRAW(const std::string & format_id, diskImage *image_to_save, const std::string & file_name = "");
        std::string get_default_ext() override;
        Result write(BYTES & buffer) override;
        Result substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}
