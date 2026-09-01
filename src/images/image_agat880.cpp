// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Agat 880 Kb FDD images (11 sectors of 512 bytes, Nippel OS)

#include "image_agat880.h"

namespace dsk_tools {
    imageAgat880::imageAgat880(std::unique_ptr<Loader> loader):
          diskImage(
              std::move(loader),
              DiskFormatParams(
                  2,                              // heads
                  80,                             // tracks
                  11,                             // sectors
                  512,                            // sector size
                  250,                            // bitrate
                  300,                            // rpm
                  ISOIBM_MFM_ENCODING,            // track encoding
                  GENERIC_SHUGGART_DD_FLOPPYMODE, // floppy interface mode
                  0                               // sector base
              )
          )
    {}

    // As for the 840 Kb disks, tracks are numbered 0..159 through both sides,
    // so a caller addresses a physical track and this maps it to a side
    uint8_t * imageAgat880::get_sector_data(unsigned head, unsigned track, unsigned sector)
    {
        return diskImage::get_sector_data(track & 1, track >> 1, sector);
    }

}
