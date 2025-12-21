// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat BFT font viewer implementation

#include <cstring>
#include "agat_fonts.h"
#include "agat_font_bft.h"

namespace dsk_tools {

    bool ViewerPicAgatFontBFT::fits(const BYTES & data)
    {
        return true;
    }

    Result ViewerPicAgatFontBFT::prepare_data(const BYTES & data, diskImage & image, fileSystem & filesystem, std::string & error_msg)
    {
        Result res = ViewerPic::prepare_data(data, image, filesystem, error_msg);
        if (!res) return res;
        constexpr int data_pos = sizeof(Agat_BFT_header) + sizeof(m_glyph_width) + sizeof(m_glyph_shift);
        if (data.size() < data_pos) return Result::error(ErrorCode::OpenBadFormat, "File is too small");
        std::memcpy(&m_bft_header, data.data(), sizeof(Agat_BFT_header));
        std::memcpy(m_glyph_width, data.data()+sizeof(Agat_BFT_header), sizeof(m_glyph_width));
        std::memcpy(m_glyph_shift, data.data()+sizeof(Agat_BFT_header)+sizeof(m_glyph_width), sizeof(m_glyph_shift));
        int glyphs_count = 0;
        m_min_shift = 0;
        int8_t max_shift = 0;
        for (unsigned i = 0; i < 256; i++) {
            m_glyph[i] = m_glyph_width[i] ? data_pos + glyphs_count++ *  m_bft_header.glyph_height : 0;
            if (m_glyph_shift[i] < m_min_shift) m_min_shift = m_glyph_shift[i];
            if (m_glyph_shift[i] > max_shift) max_shift = m_glyph_shift[i];
        }
        const int total_size = data_pos + glyphs_count + m_bft_header.glyph_height;
        if (data.size() < total_size) return Result::error(ErrorCode::OpenBadFormat, "File is too small");
        m_x_grid = 8*2 + 1;;
        m_y_grid = (m_bft_header.glyph_height - m_min_shift + max_shift) * 2 + 1;
        m_sx = (16+1) * m_x_grid; // 16 character + header
        m_sy = (16+1) * m_y_grid;

        return Result::ok();
    }

    uint32_t ViewerPicAgatFontBFT::get_pixel(int x, int y) {
        uint32_t back = 0xFF000000;
        uint32_t grid = 0xFFFF0000;
        uint32_t hdr = 0xFF00FFFF;
        uint32_t sign = 0xFFFFFFFF;
        uint32_t hint_fill = 0xFF303030;
        // int file_offset = (m_data->size() == 2048)?0:4;
        if ((x % m_x_grid ==(m_x_grid-1)) || (y % m_y_grid == (m_y_grid-1))) {
            return grid;
        }
        const int low = x / m_x_grid - 1;
        const int high = y / m_y_grid - 1;
        int line = ((y % m_y_grid) >> 1);
        const int pix = 7-((x % m_x_grid) >> 1);

        if (x < m_x_grid || y < m_y_grid) {
            // Headers
            const uint8_t (*m_font)[2048] = &A9_font;
            const int nib = (y < m_y_grid)?low:high;
            if (nib >=0) {
                int code = 0xB0 + nib;
                if (code > 0xB9) code += 7;
                line -= (m_y_grid - 16) / 4;
                if (line>=0 && line<8) {
                    const int v = ((*m_font)[code*8 + line] >> pix) & 1;
                    return v ? hdr : back;
                }
                return back;
            }
            return back;
        }
        // Character
        const int code = (high << 4) + low;
        line -= m_glyph_shift[code] - m_min_shift;
        const int ptr = m_glyph[code];
        if (ptr && pix < (7-m_glyph_width[code]))
            return back;
        if (ptr && line>=0 && line < m_bft_header.glyph_height) {
            const int v = (m_data->at(ptr + line ) >> pix) & 1;
            return v ? sign : hint_fill;
        }
        return back;
    }

    // Static registrar instantiation
    ViewerRegistrar<ViewerPicAgatFontBFT> ViewerPicAgatFontBFT::registrar;

}