// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat font viewer implementation

#include <algorithm>
#include <cctype>
#include "agat_font.h"
#include "agat_fonts.h"

namespace dsk_tools {

    PicOptions ViewerPicAgatFont::get_options()
    {
        return {
            {0,  "{$FONT_A9}"},
            {1,  "{$FONT_A7}"},
        };
    }

    int ViewerPicAgatFont::suggest_option(const std::string file_name, const BYTES & data)
    {
        std::string prefix = file_name.substr(0, 4);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        if (prefix == "ZG9_" || prefix == "ZG9:") return 0;
        if (prefix == "ZG7_" || prefix == "ZG7:") return 1;
        return -1;
    }

    bool ViewerPicAgatFont::fits(const BYTES & data)
    {
        std::vector<int> sizes_to_fit = {2048, 2048+256};
        return std::find(sizes_to_fit.begin(), sizes_to_fit.end(), data.size()) != sizes_to_fit.end();
    }

    uint32_t ViewerPicAgatFont::get_pixel(int x, int y)
    {
        uint32_t back = 0xFF000000;
        uint32_t grid = 0xFFFF0000;
        uint32_t hdr = 0xFF00FFFF;
        uint32_t sign = 0xFFFFFFFF;
        int file_offset = (m_data->size() == 2048)?0:4;
        if ((x % 15 == 14) || (y % 17 == 16)) {
            return grid;
        } else {
            int low = x / 15 - 1;
            int high = y / 17 - 1;
            int line = (y % 17) >> 1;
            int pix = ((x % 15) >> 1);

            if (x < 15 || y < 17) {
                // Headers
                const uint8_t (*m_font)[2048] = &A9_font;
                int nib = ((y < 17)?low:high);
                if (nib >=0) {
                    int code = 0xB0 + nib;
                    if (code > 0xB9) code += 7;
                    int v = ((*m_font)[code*8 + line] >> (7-pix)) & 1;
                    return (v) ? hdr : back;
                } else
                    return back;
            } else {
                // Character
                if (m_opt == 0) pix = 7 - pix;
                int code = (high << 4) + low;
                int v = (m_data->at(file_offset + code*8 + line ) >> pix) & 1;
                return (v ^ m_opt) ? sign : back;
            }
        }
    }

    // Static registrar instantiation
    ViewerRegistrar<ViewerPicAgatFont> ViewerPicAgatFont::registrar;

}
