// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat font viewer

#pragma once

#include "../viewer_pics.h"
#include "agat_common.h"

namespace dsk_tools {

    #define AGAT_FONT_TYPE_SELECTOR_ID "agat_font_type"

    class ViewerSelectorAgatFontType: public ViewerSelector {
        std::string get_id() override {return AGAT_FONT_TYPE_SELECTOR_ID;}
        std::string get_title() override {return "{$SELECTOR_AGAT_FONT_TYPE}";}
        std::string get_icon() override {return "font";}
        ViewerSelectorOptions get_options() override
        {
            ViewerSelectorOptions options;
            options.push_back({"9", "{$FONT_A9}", ""});
            options.push_back({"7", "{$FONT_A7}", ""});
            return options;
        };
    };

    class ViewerPicAgatFont : public ViewerPic {
    public:
        static ViewerRegistrar<ViewerPicAgatFont> registrar;
        ViewerPicAgatFont() {m_sx = 254; m_sy = 288;}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "FONT";}
        std::string get_subtype_text() const override {return "{$FONT_FILE}";}

        ViewerSelectors get_selectors() override;
        ViewerSelectorValues suggest_selectors(std::string file_name, const BYTES & data) override;
        void set_selectors(const ViewerSelectorValues& selectors) override;
    protected:
        bool fits(const BYTES & data) override;
        uint32_t get_pixel(int x, int y) override;
    private:
        int m_font_type = 0;
    };

}
