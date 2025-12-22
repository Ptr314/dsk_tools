// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat monochrome picture viewers implementation

#include "agat_mono.h"

namespace dsk_tools {

    uint32_t ViewerPicAgatMono::get_pixel(int x, int y)
    {
        uint32_t res = 0xFF000000;
        int i;
        if (m_sx <= 256)
            i = y * (m_sx/8) + x/8 + m_data_offset;
        else
            i = (y&1)*8192 + (y>>1) * (m_sx/8) + x/8 + m_data_offset;

        if (i < m_data->size()) {
            const uint8_t b = m_data->at(i);
            const uint8_t c = (b >> (7 - (x % 8))) & 1;
            res = convert_color(2, m_palette, c);
        }
        return res;
    }

    // Static registrar instantiations
    ViewerRegistrar<ViewerPicAgat_256x256x1> ViewerPicAgat_256x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_512x256x1> ViewerPicAgat_512x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_BMP> ViewerPicAgat_BMP::registrar;

}
