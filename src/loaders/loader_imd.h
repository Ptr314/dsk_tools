// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .IMD files (http://dunfield.classiccmp.org/img/)
#pragma once

#include "loader.h"

namespace dsk_tools {

    struct IMD_TRACK_HEADER {
        uint8_t     mode;
        uint8_t     cylinder;
        uint8_t     head;
        uint8_t     sectors;
        uint8_t     sector_size;
    };

    class LoaderIMD:public Loader
    {
    public:
        LoaderIMD(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        Result load(BYTES & buffer, const DiskFormatParams &format = DiskFormatParams()) override;
        std::string file_info() override;

    };

}
