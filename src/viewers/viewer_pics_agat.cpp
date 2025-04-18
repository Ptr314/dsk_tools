// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewers for Agat pictures

#include <cstring>
#include "definitions.h"
#include "viewer_pics_agat.h"

namespace dsk_tools {

    PicOptions ViewerPicAgat::get_options()
    {
        return {
            {0, "{$MAIN_PALETTE}"},
            {1, "{$ALT_PALETTE}"}
        };
    }

    void ViewerPicAgat::start(const BYTES & data, const int opt)
    {
        ViewerPic::start(data, opt);

        // Process Agat "EXIF"
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                if (opt == 0)
                    m_palette = exif.PALETTE >> 4;
                else
                    m_palette = exif.PALETTE & 0xF;
            }
        };
    }

    uint32_t ViewerPicAgat::convert_color(const int colors, const int palette, const int c)
    {
        int res = 0xFF000000;
        uint8_t rgb[3];

        if (colors == 2) {
            std::memcpy(&rgb, Agat_2_palette[palette & 0x3][c], 3);
        } else
        if (colors == 4) {
            std::memcpy(&rgb, Agat_4_palette[m_palette & 0x3][c], 3);
        } else
            std::memcpy(&rgb, Agat_16_palette[c], 3);

        if (palette < 4) {
            std::memcpy(&res, rgb, 3);
        } else
        if (m_palette >= 8 && m_palette <= 0xB) {
            uint8_t bw_res = (((uint32_t)rgb[0]*299 + (uint32_t)rgb[1]*587 + (uint32_t)rgb[2]*114) / 1000) & 0xFF;
            res |= (bw_res << 16) | (bw_res << 8) | bw_res;
        } else
        if (m_palette == 0xF) {
            int n = c / 2;
            int shft = (~(c & 1) & 1) * 4;
            uint8_t R = ((exif.R[n] >> shft) & 0xF) * 17;
            uint8_t G = ((exif.G[n] >> shft) & 0xF) * 17;
            uint8_t B = ((exif.B[n] >> shft) & 0xF) * 17;
            res |= (B << 16) | (G << 8) | R;
        }
        return res;
    }

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


    uint32_t ViewerPicAgatMono::get_pixel(int x, int y)
    {
        uint32_t res = 0xFF000000;
        int offset = 4;
        int i;
        if (m_sx <= 256)
            i = y * (m_sx/8) + x/8 + offset;
        else
            i = (y&1)*8192 + (y>>1) * (m_sx/8) + x/8 + offset;

        if (i < m_data->size()) {
            uint8_t b = m_data->at(i);
            uint8_t c = (b >> (7 - (x % 8))) & 1;
            res = convert_color(2, m_palette, c);
        }
        return res;
    }

    ViewerRegistrar<ViewerPicAgat_64x64x16> ViewerPicAgat_64x64x16::registrar;
    ViewerRegistrar<ViewerPicAgat_128x128x16> ViewerPicAgat_128x128x16::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x4> ViewerPicAgat_256x256x4::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x1> ViewerPicAgat_256x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_512x256x1> ViewerPicAgat_512x256x1::registrar;

}
