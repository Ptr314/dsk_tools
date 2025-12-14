// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat Apple-format picture viewers implementation

#include <cstring>
#include "agat_apple.h"

namespace dsk_tools {

    void ViewerPicAgatApple::start(const BYTES & data, const int opt, const int frame)
    {
        current_line = -1;
        ViewerPicAgat::start(data, opt, frame);
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

    PicOptions ViewerPicAgat_280x192HiRes_Agat::get_options()
    {
        return {
            {0,  "{$COLOR}"},
            {1,  "{$MONOCHROME}"},
            {15, "{$CUSTOM_PALETTE}"},
        };
    }

    void ViewerPicAgat_280x192HiRes_Agat::process_line(int line_offset, int y)
    {
        int file_offset = 4;
        static uint32_t black = 0xFF000000;
        static uint32_t white = 0xFFFFFFFF;

        const uint8_t (*palette)[16][3] = (m_opt == 0)?(&Agat_16_color):(&Agat_16_gray);

        bool prev_on = false;
        for (int x=0; x < m_sx; x++) {
            uint32_t color = 0xFF000000;

            int byte_offset = x / 7;
            int bit_offset = x % 7;
            int data_offset = line_offset + byte_offset + file_offset;
            if (data_offset < m_data->size()) {
                uint8_t b = m_data->at(data_offset);
                int hi = (b >> 7) & 1;
                int is_on = (b >> bit_offset) & 1;
                int is_odd = x & 1;
                if (is_on != 0) {
                    if (!prev_on) {
                        int c16 = agat_apple_colors_ind[is_odd][hi];
                        if (m_opt == 15) {
                            color = 0xFF000000;
                            if (exif_found) {
                                int n = c16 / 2;
                                int shft = (~(c16 & 1) & 1) * 4;
                                uint8_t R = ((exif.R[n] >> shft) & 0xF) * 17;
                                uint8_t G = ((exif.G[n] >> shft) & 0xF) * 17;
                                uint8_t B = ((exif.B[n] >> shft) & 0xF) * 17;
                                color |= (B << 16) | (G << 8) | R;
                            }
                        } else {
                            std::memcpy(&color, (*palette)[c16], 3);
                        }
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
    }

    PicOptions ViewerPicAgat_280x192HiRes_Apple::get_options()
    {
        return {
            {0, "{$NTSC_APPLE_IMPROVED}"},
            {1, "{$NTSC_APPLE_ORIGINAL}"},
            {2, "{$BW}"}
        };
    }

    void ViewerPicAgat_280x192HiRes_Apple::process_line(int line_offset, int y)
    {
        int file_offset = 4;
        static uint32_t black = 0xFF000000;
        static uint32_t white = 0xFFFFFFFF;

        bool prev_on = false;
        for (int x=0; x < m_sx; x++) {
            uint32_t color;

            int byte_offset = x / 7;
            int bit_offset = x % 7;
            int data_offset = line_offset + byte_offset + file_offset;
            if (data_offset < m_data->size()) {
                uint8_t b = m_data->at(data_offset);
                int hi = (b >> 7) & 1;
                int is_on = (b >> bit_offset) & 1;
                int is_odd = x & 1;
                if (is_on != 0) {
                    if (!prev_on) {
                        if (m_opt == 2)
                            color = white;
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
        if (m_opt == 0)
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

    PicOptions ViewerPicAgat_560x192DblHiResBW::get_options()
    {
        return {};
    }

    void ViewerPicAgat_560x192DblHiResBW::process_line(int line_offset, int y)
    {
        int file_offset = 4 + line_offset;

        for (int i = 0; i < (m_sx / 28); i++) {
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

            for (int pi = 0; pi < 28; pi++) {
                int c = (dword >> pi) & 1;
                line_data[i*28+pi] = agat_apple_hires_bw[c];
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

    // Static registrar instantiations
    ViewerRegistrar<ViewerPicAgat_280x192HiRes_Agat> ViewerPicAgat_280x192HiRes_Agat::registrar;
    ViewerRegistrar<ViewerPicAgat_280x192HiRes_Apple> ViewerPicAgat_280x192HiRes_Apple::registrar;
    ViewerRegistrar<ViewerPicAgat_140x192DblHiRes> ViewerPicAgat_140x192DblHiRes::registrar;
    ViewerRegistrar<ViewerPicAgat_560x192DblHiResBW> ViewerPicAgat_560x192DblHiResBW::registrar;
    ViewerRegistrar<ViewerPicAgat_80x48DblLoRes> ViewerPicAgat_80x48DblLoRes::registrar;
    ViewerRegistrar<ViewerPicAgat_40x48LoRes> ViewerPicAgat_40x48LoRes::registrar;

}
