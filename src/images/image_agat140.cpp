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
              DiskFormatParams(
                  1,                              // heads
                  35,                             // tracks
                  16,                             // sectors
                  256,                            // sector size
                  250,                            // bitrate
                  300,                            // rpm
                  UNKNOWN_ENCODING,               // track encoding
                  GENERIC_SHUGGART_DD_FLOPPYMODE, // floppy interface mode
                  0                               // sector base
              )
          )
    {}

    Result imageAgat140::load()
    {
        const unsigned track_size = m_format.sectors * m_format.sector_size;
        m_format.expected_size = 0;
        Result res = diskImage::load();
        const unsigned tracks = static_cast<unsigned>(m_buffer.size() / track_size);
        if (res && (m_buffer.size() % track_size != 0 || tracks < 35 || tracks > 40)) {
            m_is_loaded = false;
            res = Result::error(ErrorCode::LoadSizeMismatch, QT_TRANSLATE_NOOP("errors", "Buffer size mismatch"));
        }
        if (res) m_format.tracks = tracks;
        m_format.expected_size = m_format.heads * m_format.tracks * track_size;
        return res;
    }

}
