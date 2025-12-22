// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat monochrome picture viewers

#pragma once

#include "agat_base.h"

namespace dsk_tools {

    class ViewerPicAgatMono : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
        int m_data_offset = 4;
    };

    class ViewerPicAgat_256x256x1 : public ViewerPicAgatMono {
    public:
        static ViewerRegistrar<ViewerPicAgat_256x256x1> registrar;

        ViewerPicAgat_256x256x1() {m_sx = 256; m_sy = 256; m_sizes_to_fit = {8192, 8192+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "256x256x1";}
        std::string get_subtype_text() const override {return "256х256x2 МГВР";}
    };

    class ViewerPicAgat_BMP : public ViewerPicAgatMono {
    public:
        static ViewerRegistrar<ViewerPicAgat_BMP> registrar;

        ViewerPicAgat_BMP() {m_sx = 256; m_sy = 256; m_sizes_to_fit = {8192}; m_data_offset=0;}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "AGAT_BMP";}
        std::string get_subtype_text() const override {return "256х256x2 BMP";}
    };

    class ViewerPicAgat_512x256x1 : public ViewerPicAgatMono {
    public:
        static ViewerRegistrar<ViewerPicAgat_512x256x1> registrar;

        ViewerPicAgat_512x256x1() {m_sx = 512; m_sy = 256; m_sizes_to_fit = {16384, 16384+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "512x256x1";}
        std::string get_subtype_text() const override {return "512х256x2 МГДП";}
    };

}
