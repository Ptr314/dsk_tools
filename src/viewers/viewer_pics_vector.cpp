// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewers for Vector-06C pictures

#include <cstring>

#include "viewer_pics_vector.h"
#include "utils.h"

namespace dsk_tools {

    bool ViewerPicVectorSPR::fits(const BYTES & data, const std::string & file_name)
    {
        return get_file_ext(file_name) == ".spr";
    }

    Result ViewerPicVectorSPR::prepare_data(const BYTES & data, diskImage & image, fileSystem & filesystem, std::string & error_msg)
    {
        Result res = ViewerPic::prepare_data(data, image, filesystem, error_msg);
        if (!res) return res;

        // https://github.com/drilnet/vector-06c-spr2bmp/wiki

        for (int i=0; i<16; i++) {
            uint8_t v = data[i];
            uint8_t R = v & 0b00000111;
            uint8_t G = (v & 0b00111000) >> 3;
            uint8_t B = (v & 0b11000000) >> 6;

            m_palette[i] = 0xFF000000 | (B*85 << 16) | ((G*255/7) << 8) | (R*255/7);
        }

        // Data parsing starts from the end
        unsigned ps = 32768;
        unsigned pd = data.size();

        while (pd>0 && data[--pd]==0) {}; // Skipping trailing zeroes

        while (pd > 0) {
            const uint8_t b = data[pd--];
            const uint8_t c = b & 0x7F;
            if (ps < c) break;
            if (b & 0x80) {
                // c copies of the next byte
                if (pd == 0 && c > 0) break;
                const uint8_t b1 = data[pd--];
                ps -= c;
                std::memset(&m_screen[ps], b1, c);
            } else {
                // c following bytes in backwards order
                if (pd + 1 < c) break;
                for (uint8_t i = 0; i < c; ++i)
                    m_screen[--ps] = data[pd--];
            }
        }

        return Result::ok();
    }

    uint32_t ViewerPicVectorSPR::get_pixel(int x, int y)
    {
        // Vector-06C screen: 4 bit-planes of 8192 bytes laid out sequentially.
        // Each plane: 32 columns x 256 bytes, 8 horizontal pixels per byte,
        // bytes within a column grow bottom-to-top, bit 7 is the leftmost pixel.
        constexpr int PLANE_SIZE = 8192;
        const int byte_offset = (x >> 3) * 256 + (255 - y);
        const int bit = 7 - (x & 7);

        uint8_t color = 0;
        for (int p = 0; p < 4; ++p)
            color |= ((m_screen[p * PLANE_SIZE + byte_offset] >> bit) & 1) << (3-p);

        return m_palette[color];
    }
}
