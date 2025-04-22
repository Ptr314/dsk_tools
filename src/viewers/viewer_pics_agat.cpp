// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewers for Agat pictures

#include <cstring>
#include <algorithm>
#include <iostream>
#include <ostream>
#include "definitions.h"
#include "viewer_pics_agat.h"
#include "agat_fonts.h"
#include "fs_dos33.h"
#include "loader_fil.h"
#include "utils.h"

namespace dsk_tools {

    PicOptions ViewerPicAgat::get_options()
    {
        return {
            {0,  "{$PALETTE} #1"},
            {1,  "{$PALETTE} #2"},
            {2,  "{$PALETTE} #3"},
            {3,  "{$PALETTE} #4"},
            {8,  "{$PALETTE} #1 {$BW}"},
            {9,  "{$PALETTE} #2 {$BW}"},
            {10, "{$PALETTE} #3 {$BW}"},
            {11, "{$PALETTE} #4 {$BW}"},
            {15, "{$CUSTOM_PALETTE}"},
        };
    }

    int ViewerPicAgat::suggest_option(const BYTES & data)
    {
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                return exif.PALETTE >> 4;
            }
        }
        return -1;
    }

    void ViewerPicAgat::start(const BYTES & data, const int opt, const int frame)
    {
        ViewerPic::start(data, opt, frame);

        // Process Agat "EXIF"
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                m_palette = opt;
            }
        };
    }

    bool ViewerPicAgat::fits(const BYTES & data)
    {
        if (m_sizes_to_fit.size() != 0)
            return std::find(m_sizes_to_fit.begin(), m_sizes_to_fit.end(), data.size()) != m_sizes_to_fit.end();
        else
            return true;
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

    PicOptions ViewerPicAgatText::get_options()
    {
        return {
            {0, "{$AGAT_FONT_A7_CLASSIC}"},
            {1, "{$AGAT_FONT_A7_ENCHANCED}"},
            {2, "{$AGAT_FONT_A9_CLASSIC}"},
            // {3, "{$AGAT_FONT_CUSTOM_GARNIZON}"},
            {100, "{$AGAT_FONT_CUSTOM_LOADED}"}
        };
    }

    int ViewerPicAgatText::suggest_option(const BYTES & data)
    {
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                int font = exif.FONT >> 4;
                if (font == 7) return 0;
                if (font == 8) return 1;
                if (font == 9) return 2;
                if (font == 0xF) {
                    // std::string font_name = to_upper(trim(agat_to_utf(exif.FONT_NAME, 15)));
                    // if (font_name == "GARNIZON") return 3;
                    return 100;
                }
            }
        }
        return -1;
    }

    void ViewerPicAgatText::start(const BYTES & data, const int opt, const int frame)
    {
        current_line = -1;
        ViewerPicAgat::start(data, opt, frame);

        switch (opt) {
            case 1:
                m_font = &A7_font_svt;
                m_font_reverse = false;
                break;
            case 2:
                m_font = &A9_font;
                m_font_reverse = true;
                break;
            // case 3:
            //     m_font = &Agat_garnizon_font;
            //     m_font_reverse = true;
            //     break;
            case 100:
                m_font = &m_custom_font;
                m_font_reverse = m_custom_reverse;
                break;
            default:
                m_font = &A7_font;
                m_font_reverse = false;
                break;
        }

    }

    int ViewerPicAgatText::prepare_data(const BYTES & data, dsk_tools::diskImage & image, dsk_tools::fileSystem & filesystem, std::string & error_msg)
    {
        error_msg = "";
        std::memset(&m_custom_font, 0xAA, sizeof(m_custom_font));

        int res = ViewerPicAgat::prepare_data(data, image, filesystem, error_msg);
        if (res != PREPARE_PIC_OK) return res;

        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                int font = exif.FONT >> 4;
                if (font == 0xF) {
                    std::string font_name = to_upper(trim(agat_to_utf(exif.FONT_NAME, 15)));

                    BYTES buffer;
                    std::string file_name;

                    // Trying to find on a loaded filesystem
                    fileData fd;
                    file_name = "ZG9:"+font_name;
                    if (filesystem.file_find(file_name,fd)) {
                        buffer = filesystem.get_file(fd);
                        m_custom_reverse = true;
                        std::memcpy(&m_custom_font, buffer.data()+4, 2048);
                        return PREPARE_PIC_OK;
                    }
                    file_name = "ZG7:"+font_name;
                    if (filesystem.file_find(file_name,fd)) {
                        buffer = filesystem.get_file(fd);
                        m_custom_reverse = false;
                        std::memcpy(&m_custom_font, buffer.data()+4, 2048);
                        return PREPARE_PIC_OK;
                    }

                    // Trying to find on a host
                    std::string file_path = get_file_path(image.file_name());
                    file_name = file_path + font_name + ".fil";
                    if (file_exists(file_name)) {
                        LoaderFIL loader(file_name, "", "");
                        loader.load(buffer);
                        FIL_header * header = reinterpret_cast<FIL_header*>(buffer.data());
                        std::string internal_name = to_upper(trim(agat_to_utf(header->name, 30)));
                        if (internal_name.substr(0, 2) == "ZG") {
                            m_custom_reverse = internal_name.substr(2, 1) == "9";
                            std::memcpy(&m_custom_font, buffer.data()+44, 2048);
                            return PREPARE_PIC_OK;
                        }
                    }
                    error_msg = "{$FONT_LOADING_ERROR}: " + font_name;
                    return PREPARE_PIC_ERROR;
                }
            }
        }
        return PREPARE_PIC_OK;
    }

    bool ViewerPicAgatText::load_custom_font()
    {
        std::string font_name = to_upper(trim(agat_to_utf(exif.FONT_NAME, 15)));

        return false;
    }

    uint32_t ViewerPicAgatText::get_pixel(int x, int y)
    {
        if (current_line != y) {
            process_line(y);
            current_line = y;
        }
        return line_data[x];
    }

    void ViewerPicAgatTextT32::process_line(int y)
    {
        static uint32_t black = 0xFF000000;

        // 32 pixels blanking of sides
        for (int i=0; i<16; i++) {
            line_data[i] = black;
            line_data[i+240] = black;
        }

        bool blinker = m_frame & 1;

        int file_offset = 4;

        int line_num = y >> 3;
        int font_lin = y & 7;
        for (int char_num = 0; char_num < 32; char_num++) {
            int index = file_offset + line_num*64 + char_num*2;
            if (index + 1 < m_data->size()) {
                uint8_t v1 = m_data->at(index);                 // Character
                uint8_t v2 = m_data->at(index+1);               // Attribute
                uint8_t cl = (v2 & 0x07) | ((v2 & 0x10) >> 1);  // Color index (YBGR)
                for (int k = 0; k < 7; k++) {
                    uint8_t font_val = (*m_font)[v1*8+font_lin];
                    if (!m_font_reverse)
                        font_val = ~font_val;
                    uint8_t c = ((m_font_reverse)?(font_val >> (7-k)):(font_val >> k)) & 0x01;
                    uint8_t ccl;
                    if ( (((v2 & 0x20) != 0) || ((v2 & 0x08) != 0) && blinker) )
                    // if ( (v2 & 0x20) != 0 )
                        ccl = cl * c;
                    else
                        ccl = cl * (c ^ 0x01);
                    uint32_t color = black;
                    std::memcpy(&color, &(Agat_16_color[ccl]), 3);
                    int pi = char_num*7 + k;
                    line_data[16 + pi] = color;
                }
            }
        }
    }

    void ViewerPicAgatTextT64::process_line(int y)
    {
        static uint32_t black = 0xFF000000;

        // 32 pixels blanking of sides
        for (int i=0; i<32; i++) {
            line_data[i] = black;
            line_data[i+480] = black;
        }

        int file_offset = 4;

        int line_num = y >> 3;
        int font_lin = y & 7;
        for (int char_num = 0; char_num < 64; char_num++) {
            int index = file_offset + line_num*64 + char_num;
            if (index < m_data->size()) {
                uint8_t v1 = m_data->at(index);                 // Character
                for (int k = 0; k < 7; k++) {
                    uint8_t font_val = (*m_font)[v1*8+font_lin] ;
                    if (!m_font_reverse)
                        font_val = ~font_val;
                    uint8_t c = (((m_font_reverse)?(font_val >> (7-k)):(font_val >> k)) & 0x01) * 15;
                    uint32_t color = black;
                    std::memcpy(&color, &(Agat_16_color[c]), 3);
                    int pi = char_num*7 + k;
                    line_data[32 + pi] = color;
                }
            }
        }
    }

    ViewerRegistrar<ViewerPicAgat_64x64x16> ViewerPicAgat_64x64x16::registrar;
    ViewerRegistrar<ViewerPicAgat_128x128x16> ViewerPicAgat_128x128x16::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x4> ViewerPicAgat_256x256x4::registrar;
    ViewerRegistrar<ViewerPicAgat_256x256x1> ViewerPicAgat_256x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_512x256x1> ViewerPicAgat_512x256x1::registrar;
    ViewerRegistrar<ViewerPicAgat_128x256x16> ViewerPicAgat_128x256x16::registrar;

    ViewerRegistrar<ViewerPicAgatTextT32> ViewerPicAgatTextT32::registrar;
    ViewerRegistrar<ViewerPicAgatTextT64> ViewerPicAgatTextT64::registrar;

    ViewerRegistrar<ViewerPicAgat_280x192HiRes> ViewerPicAgat_280x192HiRes::registrar;
    ViewerRegistrar<ViewerPicAgat_140x192DblHiRes> ViewerPicAgat_140x192DblHiRes::registrar;
    ViewerRegistrar<ViewerPicAgat_560x192DblHiResBW> ViewerPicAgat_560x192DblHiResBW::registrar;
    ViewerRegistrar<ViewerPicAgat_80x48DblLoRes> ViewerPicAgat_80x48DblLoRes::registrar;
    ViewerRegistrar<ViewerPicAgat_40x48LoRes> ViewerPicAgat_40x48LoRes::registrar;

}
