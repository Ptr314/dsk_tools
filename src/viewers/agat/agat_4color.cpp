// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat 4-color picture viewer implementation

#include "agat_4color.h"

namespace dsk_tools {

    uint32_t ViewerPicAgat4::get_pixel(int x, int y)
    {
        uint32_t res = 0xFF000000;
        int offset = 4;
        int i = (y&1)*8192 + (y>>1) * (m_sx/4) + x/4 + offset;

        if (i < m_data->size()) {
            uint8_t b = m_data->at(i);
            uint8_t c = (b >> (3 - (x % 4))*2) & 0x3;
            res = convert_color(4, m_palette, c);
        }
        return res;
    }

    // Static registrar instantiation
    ViewerRegistrar<ViewerPicAgat_256x256x4> ViewerPicAgat_256x256x4::registrar;

}
