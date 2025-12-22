// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat BFT font viewer

#pragma once

#include "../viewer_pics.h"

namespace dsk_tools {

    struct Agat_BFT_header {
        uint8_t     _unknown_00[5];     //00 - 86 00 40 00 08
        uint8_t     glyph_height;       //06
        uint8_t     _unknown_07;        //07 - 01
        uint8_t     _unknown_08;        //08 - 0C or 0D
        uint8_t     _unknown_09[8];     //09 - 00 00 00 00 00 00 00 00
        uint8_t     _unknown_10[16];    //10 - 20 00 40 00 60 00 00 00 60 01 60 02 00 00 00 00
        uint8_t     font_name[16];      //20
        uint8_t     _unknown_30[16];    //30 - zeroes
        uint8_t     _unknown_40[16];    //30 - zeroes
        uint8_t     _unknown_50[16];    //30 - zeroes
    };

    class ViewerPicAgatFontBFT : public ViewerPic {
    public:
        static ViewerRegistrar<ViewerPicAgatFontBFT> registrar;
        ViewerPicAgatFontBFT() {m_sx = 100; m_sy = 100;}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "AGAT_BFT";}
        std::string get_subtype_text() const override {return "{$FONT_FILE_BFT}";}

        Result prepare_data(const BYTES & data, diskImage & image, fileSystem & filesystem, std::string & error_msg) override;

    protected:
        bool fits(const BYTES & data) override;
        uint32_t get_pixel(int x, int y) override;
    private:
        Agat_BFT_header     m_bft_header;
        uint8_t             m_glyph_width[256];
        int8_t              m_glyph_shift[256];
        int8_t              m_min_shift = 0;
        int                 m_glyph[256];
        int                 m_x_grid = 0;
        int                 m_y_grid = 0;
    };

}
