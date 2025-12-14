// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat 16-color picture viewers

#pragma once

#include "agat_base.h"

namespace dsk_tools {

    class ViewerPicAgat16 : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_64x64x16 : public ViewerPicAgat16 {
    public:
        static ViewerRegistrar<ViewerPicAgat_64x64x16> registrar;

        ViewerPicAgat_64x64x16() {m_sx = 64; m_sy = 64; m_sizes_to_fit = {2048, 2048+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "64x64x16";}
        std::string get_subtype_text() const override {return "64x64x16 ЦГНР";}
    };

    class ViewerPicAgat_128x128x16 : public ViewerPicAgat16 {
    public:
        static ViewerRegistrar<ViewerPicAgat_128x128x16> registrar;

        ViewerPicAgat_128x128x16() {m_sx = 128; m_sy = 128; m_sizes_to_fit = {8192, 8192+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "128x128x16";}
        std::string get_subtype_text() const override {return "128x128x16 ЦГСР";}
    };

    class ViewerPicAgat_128x256x16 : public ViewerPicAgat16 {
    public:
        static ViewerRegistrar<ViewerPicAgat_128x256x16> registrar;

        ViewerPicAgat_128x256x16() {m_sx = 128; m_sy = 256; m_sizes_to_fit = {16384, 16384+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "128x256x16";}
        std::string get_subtype_text() const override {return "128x256 16 цветов";}
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

}
