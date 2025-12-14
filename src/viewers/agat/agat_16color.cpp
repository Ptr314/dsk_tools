// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat 16-color picture viewers implementation

#include "agat_16color.h"

namespace dsk_tools {

    uint32_t ViewerPicAgat16::get_pixel(int x, int y)
    {
        uint32_t res = 0xFF000000;
        int offset = 4;
        int i = y * (m_sx/2) + x/2 + offset;

        if (i < m_data->size()) {
            uint8_t b = m_data->at(i);
            uint8_t c = (x&1)?(b&0xF):(b>>4);
            res = convert_color(16, m_palette, c);
        }
        return res;
    }

    uint32_t ViewerPicAgat_128x256x16::get_pixel(int x, int y)
    {
        uint32_t res = 0xFF000000;
        int offset = 4;
        int i;
        if ((y&1) == 0)
            i = (y>>1) * (m_sx/2) + x/2 + offset;
        else
            i = (y>>1) * (m_sx/2) + x/2 + offset + 8192;

        if (i < m_data->size()) {
            uint8_t b = m_data->at(i);
            uint8_t c = (x&1)?(b&0xF):(b>>4);
            res = convert_color(16, m_palette, c);
        }
        return res;
    }

    // Static registrar instantiations
    ViewerRegistrar<ViewerPicAgat_64x64x16> ViewerPicAgat_64x64x16::registrar;
    ViewerRegistrar<ViewerPicAgat_128x128x16> ViewerPicAgat_128x128x16::registrar;
    ViewerRegistrar<ViewerPicAgat_128x256x16> ViewerPicAgat_128x256x16::registrar;

}
