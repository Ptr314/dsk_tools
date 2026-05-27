// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewers for Vector-06C pictures

#pragma once

#include "viewer_pics.h"

namespace dsk_tools {

    // static const uint8_t Vector_16_color[16][3]  = {
    //     {  0,   0,   0}, {217,   0,   0}, {  0, 217,   0}, {217, 217,   0},
    //     {  0,   0, 217}, {217,   0, 217}, {  0, 217, 217}, {217, 217, 217},
    //     { 38,  38, 	38}, {255,  38,  38}, { 38, 255,  38}, {255, 255,  38},
    //     { 38,  38, 255}, {255,  38, 255}, { 38, 255, 255}, {255, 255, 255}
    // };

    class ViewerPicVectorSPR : public ViewerPic {
    protected:
        uint32_t get_pixel(int x, int y) override;
        uint8_t m_screen[32768];
        uint32_t m_palette[16];
    public:
        static ViewerRegistrar<ViewerPicVectorSPR> registrar;

        ViewerPicVectorSPR() {m_sx = 256; m_sy = 256;}

        std::string get_type() const override {return "PICTURE_VECTOR";}
        std::string get_subtype() const override {return "SPR";}
        std::string get_subtype_text() const override {return "SPR";}
        bool fits(const BYTES & data, const std::string & file_name) override;
        Result prepare_data(const BYTES & data, diskImage & image, fileSystem & filesystem, std::string & error_msg) override;
    };

}
