// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for IBM PC 360 images

#include "loader.h"
#include "image_pc360.h"

namespace dsk_tools {
    imagePC360::imagePC360(std::unique_ptr<Loader> loader, const bool is_interleaved):
              diskImage(std::move(loader))
    {
        format_heads = 2;
        format_tracks = 40;
        format_sectors = 9;
        format_sector_size = 512;
        expected_size = format_heads * format_tracks * format_sectors * format_sector_size;
        format_bitrate = 250;
        format_rpm = 300;
        format_track_encoding = UNKNOWN_ENCODING;
        format_floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
        interleaved = is_interleaved;
    }

    Result imagePC360::check()
    {
        // TODO: Implement validation
        return Result::ok();
    }

}
