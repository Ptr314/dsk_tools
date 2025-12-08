// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - image part

#include "loader.h"
#include "image_fil.h"

namespace dsk_tools {
    imageFIL::imageFIL(std::unique_ptr<Loader> loader):
        diskImage(std::move(loader))
    {
        format_heads = 1;
        format_tracks = 1;
        format_sectors = 1;
        format_sector_size = 256;
        expected_size = 0;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    int imageFIL::check()
    {
        return FDD_LOAD_OK;
    }

    int imageFIL::load()
    {
        int res = diskImage::load();
        if (res == FDD_LOAD_OK)
            expected_size = buffer.size();
        return res;
    }

}
