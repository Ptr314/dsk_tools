// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat 4-color picture viewer

#pragma once

#include "agat_base.h"

namespace dsk_tools {

    class ViewerPicAgat4 : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_256x256x4 : public ViewerPicAgat4 {
    public:
        static ViewerRegistrar<ViewerPicAgat_256x256x4> registrar;

        ViewerPicAgat_256x256x4() {m_sx = 256; m_sy = 256; m_sizes_to_fit = {16384, 16384+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "256x256x4";}
        std::string get_subtype_text() const override {return "256х256x4 ЦГВР";}
    };

}
