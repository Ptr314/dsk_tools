// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Agat 840 Kb FDD images

#include "image_agat840.h"

namespace dsk_tools {
    imageAgat840::imageAgat840(std::unique_ptr<Loader> loader):
        imageAgat140(std::move(loader))
    {
        m_format.heads = 2;
        m_format.tracks = 80;
        m_format.sectors = 21;
        m_format.expected_size = m_format.heads * m_format.tracks * m_format.sectors * m_format.sector_size;
        m_format.track_encoding = ISOIBM_MFM_ENCODING;
        m_format.floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    }

    uint8_t * imageAgat840::get_sector_data(unsigned head, unsigned track, unsigned sector)
    {
        return diskImage::get_sector_data(track & 1, track >> 1, sector);
    }


}
