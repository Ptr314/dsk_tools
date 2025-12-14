// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat font viewer implementation

#include <algorithm>
#include <cctype>
#include "agat_font.h"
#include "agat_fonts.h"
#include "utils.h"

namespace dsk_tools {

    ViewerSelectorValues ViewerPicAgatFont::suggest_selectors(const std::string file_name, const BYTES & data)
    {
        ViewerSelectorValues result;
        std::string prefix = file_name.substr(0, 4);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        if (prefix == "ZG9_" || prefix == "ZG9:") result[AGAT_FONT_TYPE_SELECTOR_ID] = "9";
        if (prefix == "ZG7_" || prefix == "ZG7:") result[AGAT_FONT_TYPE_SELECTOR_ID] = "7";
        return result;
    }

    bool ViewerPicAgatFont::fits(const BYTES & data)
    {
        std::vector<int> sizes_to_fit = {2048, 2048+256};
        return std::find(sizes_to_fit.begin(), sizes_to_fit.end(), data.size()) != sizes_to_fit.end();
    }

    void ViewerPicAgatFont::set_selectors(const ViewerSelectorValues& selectors) {
        ViewerPic::set_selectors(selectors);
        m_font_type = m_selectors[AGAT_FONT_TYPE_SELECTOR_ID] == "9" ? 0 : 1;
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
                if (m_font_type == 0) pix = 7 - pix;
                int code = (high << 4) + low;
                int v = (m_data->at(file_offset + code*8 + line ) >> pix) & 1;
                return (v ^ m_font_type) ? sign : back;
            }
        }
    }

    ViewerSelectors ViewerPicAgatFont::get_selectors()
    {
        std::vector<std::unique_ptr<ViewerSelector>> result;
        result.push_back(make_unique<ViewerSelectorAgatFontType>());
        return result;
    }

    // Static registrar instantiation
    ViewerRegistrar<ViewerPicAgatFont> ViewerPicAgatFont::registrar;

}
