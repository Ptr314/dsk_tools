// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Apple II / Agat 140 Kb FDD images

#include "loader.h"
#include "image_agat140.h"

namespace dsk_tools {
imageAgat140::imageAgat140(std::unique_ptr<Loader> loader):
          diskImage(
              std::move(loader),
              1,                              // heads
              35,                             // tracks
              16,                             // sectors
              256,                            // sector size
              250,                            // bitrate
              300,                            // rpm
              UNKNOWN_ENCODING,               // track encoding
              GENERIC_SHUGGART_DD_FLOPPYMODE  // floppy interface mode
          )
    {}

}
