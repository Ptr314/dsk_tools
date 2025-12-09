// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Apple II / Agat 140 Kb FDD images

#include "loader.h"
#include "image_agat140.h"

namespace dsk_tools {
imageAgat140::imageAgat140(std::unique_ptr<Loader> loader):
          diskImage(std::move(loader))
    {
        format_heads = 1;
        format_tracks = 35;
        format_sectors = 16;
        format_sector_size = 256;
        expected_size = format_heads * format_tracks * format_sectors * format_sector_size;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    Result imageAgat140::check()
    {
        // TODO: Implement validation
        return Result::ok();
    }

}
