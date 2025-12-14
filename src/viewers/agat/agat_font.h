// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat font viewer

#pragma once

#include "../viewer_pics.h"
#include "agat_common.h"

namespace dsk_tools {

    class ViewerPicAgatFont : public ViewerPic {
    public:
        static ViewerRegistrar<ViewerPicAgatFont> registrar;
        ViewerPicAgatFont() {m_sx = 254; m_sy = 288;}

        PicOptions get_options() override;
        int suggest_option(const std::string file_name, const BYTES & data) override;
        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "FONT";}
        std::string get_subtype_text() const override {return "{$FONT_FILE}";}
    protected:
        bool fits(const BYTES & data) override;
        uint32_t get_pixel(int x, int y) override;
    };

}
