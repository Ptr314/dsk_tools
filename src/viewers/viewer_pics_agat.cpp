// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewers for Agat pictures

#include <cstring>
#include <iostream>
#include <ostream>
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

    uint32_t ViewerPicAgat::convert_color(const int colors, const int palette_id, const int c)
    {
        int res = 0xFF000000; // ABGR (little-endian RGBA in memory)

        // https://agatcomp.ru/agat/Hardware/useful/ColorSet.shtml
        // https://agatcomp.ru/agat/PCutils/EXIF.shtml
        // 0-3: standard palette
        // 8-B: grayscale pallette
        //   F: custom palette from EXIF

        // Firstly we choose from standard palettes
        const uint8_t (*palette)[16][3] = &Agat_16_color;
        if (palette_id >=0x8 && palette_id <= 0xB)
            palette = &Agat_16_gray;

        // Each color maps to a 16-color palette (color, grayscale or custom)
        int c16 = c;
        if (palette_id < 0xF) {
            // Standard palettes
            if (colors == 2)
                c16 = Agat_2_index[palette_id & 0x3][c];
            else
            if (colors == 4)
                c16 = Agat_4_index[palette_id & 0x3][c];
            std::memcpy(&res, (*palette)[c16], 3);
        } else {
            // Custom palette from EXIF
            if (colors == 2)
                c16 = Agat_2_index[0][c];
            else
            if (colors == 4)
                c16 = Agat_4_index[0][c];

            // Each palette byte stores two 4-bit values
            int n = c16 / 2;                                // Position in the custom palette
            int shft = (~(c16 & 1) & 1) * 4;                // 0 or 4
            uint8_t R = ((exif.R[n] >> shft) & 0xF) * 17;   // Map 0-F to 00-FF
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

    PicOptions ViewerPicAgat_280x192HiRes::get_options()
    {
        return {
            {0, "{$NTSC_AGAT_IMPROVED}"},
            {1, "{$NTSC_APPLE_IMPROVED}"},
            {2, "{$NTSC_APPLE_ORIGINAL}"}
        };

    }

    void ViewerPicAgatApple::start(const BYTES & data, const int opt)
    {
        current_line = -1;
        ViewerPicAgat::start(data, opt);
    }

    uint32_t ViewerPicAgatApple::get_pixel(int x, int y)
    {
        int bytesPerSuperblock = 1024;
        int bytesPerBlock = 128;
        int bytesPerLine = 40;

        int superblock = y & 7;
        int block = (y >> 3) & 7;
        int lineInBlock = (y >> 6) & 3;
        int line_offset = bytesPerSuperblock * superblock + bytesPerBlock * block + bytesPerLine * lineInBlock;

        if (current_line != y) {
            process_line(line_offset);
            current_line = y;
        }
        return line_data[x];
    }

    void ViewerPicAgat_280x192HiRes::process_line(int line_offset, int y)
    {
        int file_offset = 4;
        static uint32_t black = 0xFF000000;
        static uint32_t white = 0xFFFFFFFF;

        bool prev_on = false;
        for (int x=0; x < m_sx; x++) {
            uint32_t color;

            int byte_offset = x / 7;
            int bit_offset = x % 7; // 6 - (x % 7);
            int data_offset = line_offset + byte_offset + file_offset;
            if (data_offset < m_data->size()) {
                uint8_t b = m_data->at(data_offset);
                int hi = (b >> 7) & 1;
                int is_on = (b >> bit_offset) & 1;
                int is_odd = x & 1;
                if (is_on != 0) {
                    if (!prev_on) {
                        if (m_opt == 0)
                            color = agat_apple_colors[is_odd][hi];
                        else
                            color = agat_apple_colors_NTSC[is_odd][hi];
                    } else {
                        color = white;
                        line_data[x-1] = white;
                    }
                    prev_on = true;
                } else {
                    color = black;
                    prev_on = false;
                }
            } else {
                color = black;
                prev_on = false;
            }
            line_data[x] = color;
        }
        if (m_opt == 0 || m_opt == 1)
            for (int x=1; x < m_sx-1; x++) {
                if (line_data[x] == black && line_data[x-1] == line_data[x+1]) line_data[x] = line_data[x-1];
            }
    }

    PicOptions ViewerPicAgat_140x192DblHiRes::get_options()
    {
        return {};

    }

    void ViewerPicAgat_140x192DblHiRes::process_line(int line_offset, int y)
    {
        int file_offset = 4 + line_offset;

        for (int i = 0; i < (m_sx / 7); i++) {
            int offset = file_offset + i*2;
            uint32_t dword = 0;

            if (offset < m_data->size())
                dword |= (m_data->at(offset) & 0x7F);
            if (offset + 8192 < m_data->size())
                dword |= ((m_data->at(offset + 8192) & 0x7F) << 7);
            if (offset + 1 < m_data->size())
                dword |= ((m_data->at(offset + 1) & 0x7F) << 14);
            if (offset + 8193 < m_data->size())
                dword |= ((m_data->at(offset + 8193) & 0x7F) << 21);

            for (int pi = 0; pi < 7; pi++) {
                int c = (dword >> (pi*4)) & 0xF;
                line_data[i*7+pi] = agat_apple_hires_colors[c];
            }
        }
    }

    PicOptions ViewerPicAgat_40x48LoRes::get_options()
    {
        return {};

    }

    uint32_t ViewerPicAgat_40x48LoRes::get_pixel(int x, int y)
    {
        int bytesPerBlock = 128;
        int bytesPerLine = 40;

        int block = (y >> 1) & 7;
        int lineInBlock = (y >> 4) & 3;
        int line_offset = bytesPerBlock * block + bytesPerLine * lineInBlock;

        if (current_line != y) {
            process_line(line_offset, y);
            current_line = y;
        }
        return line_data[x];
    }

    void ViewerPicAgat_40x48LoRes::process_line(int line_offset, int y)
    {
        static uint32_t black = 0xFF000000;

        int file_offset = 4 + line_offset;
        for (int x=0; x<40; x++) {
            int data_offset = file_offset + x;
            if (data_offset < m_data->size()) {
                uint8_t b = m_data->at(data_offset);
                int c = (y&1)?b>>4:b&0xF;
                line_data[x] = agat_apple_lores_colors[c];
            } else {
                line_data[x] = black;
            }
        }
    }

    void ViewerPicAgat_80x48DblLoRes::process_line(int line_offset, int y)
    {
        static uint32_t black = 0xFF000000;

        int file_offset = 4 + line_offset;
        for (int x=0; x<40; x++) {
            int data_offset_1 = file_offset + x;
            if (data_offset_1 < m_data->size()) {
                uint8_t b1 = m_data->at(data_offset_1);
                int c1 = (y&1)?b1>>4:b1&0xF;
                line_data[x*2+1] = agat_apple_lores_colors[c1];
            } else {
                line_data[x*2+1] = black;
            }

            int data_offset_2 = file_offset + x + 1024;
            if (data_offset_2 < m_data->size()) {
                uint8_t b2 = m_data->at(data_offset_2);
                int c2 = (y&1)?b2>>4:b2&0xF;
                line_data[x*2] = agat_apple_lores_colors[c2];
            } else {
                line_data[x*2] = black;
            }
        }

    }

    ViewerRegistrar<ViewerPicAgat_64x64x16> ViewerPicAgat_64x64x16::registrar;
    ViewerRegistrar<ViewerPicAgat_128x128x16> ViewerPicAgat_128x128x16::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x4> ViewerPicAgat_256x256x4::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x1> ViewerPicAgat_256x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_512x256x1> ViewerPicAgat_512x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_128x256x16> ViewerPicAgat_128x256x16::registrar;

    ViewerRegistrar<ViewerPicAgat_280x192HiRes> ViewerPicAgat_280x192HiRes::registrar;
    ViewerRegistrar<ViewerPicAgat_140x192DblHiRes> ViewerPicAgat_140x192DblHiRes::registrar;
    ViewerRegistrar<ViewerPicAgat_80x48DblLoRes> ViewerPicAgat_80x48DblLoRes::registrar;
    ViewerRegistrar<ViewerPicAgat_40x48LoRes> ViewerPicAgat_40x48LoRes::registrar;

}
