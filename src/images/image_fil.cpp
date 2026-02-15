// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - image part

#include "loader.h"
#include "image_fil.h"

namespace dsk_tools {
    imageFIL::imageFIL(std::unique_ptr<Loader> loader):
        diskImage(
            std::move(loader),
            DiskFormatParams(
                1,                              // heads
                1,                              // tracks
                1,                              // sectors
                256,                            // sector size
                250,                            // bitrate
                300,                            // rpm
                UNKNOWN_ENCODING,               // track encoding
                GENERIC_SHUGGART_DD_FLOPPYMODE, // floppy interface mode
                0
            )
        )
    {
        m_format.expected_size = 0;
    }

    Result imageFIL::load()
    {
        Result res = diskImage::load();
        if (res)
            m_format.expected_size = m_buffer.size();
        return res;
    }

}
