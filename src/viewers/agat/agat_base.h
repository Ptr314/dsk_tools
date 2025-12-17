// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Base Agat picture viewer

#pragma once

#include <cstring>
#include <algorithm>
#include <vector>
#include "../viewer_pics.h"
#include "agat_common.h"

namespace dsk_tools {

    #define AGAT_PALETTE_SELECTOR_ID "agat_palette"

    class ViewerSelectorAgatPalette: public ViewerSelector {
        std::string get_id() override {return AGAT_PALETTE_SELECTOR_ID;}
        std::string get_title() override {return "{$SELECTOR_AGAT_PALETTE}";}
        std::string get_icon() override {return "palette";}
        bool has_customs() override {return true;}
        ViewerSelectorOptions get_options() override
        {
            ViewerSelectorOptions options;
            options.push_back({"0", "{$PALETTE} #1", ""});
            options.push_back({"1", "{$PALETTE} #2", ""});
            options.push_back({"2", "{$PALETTE} #3", ""});
            options.push_back({"3", "{$PALETTE} #4", ""});
            options.push_back({"8", "{$PALETTE} #1 {$BW}", ""});
            options.push_back({"9", "{$PALETTE} #2 {$BW}", ""});
            options.push_back({"10", "{$PALETTE} #3 {$BW}", ""});
            options.push_back({"11", "{$PALETTE} #4 {$BW}", ""});
            options.push_back({"15", "{$CUSTOM_PALETTE}", ""});
            return options;
        };
    };

    #define AGAT_INFO_SELECTOR_ID "agat_info"

    class ViewerSelectorAgatInfo: public ViewerSelector {
        std::string get_id() override {return AGAT_INFO_SELECTOR_ID;}
        std::string get_type() override {return "info";}
        std::string get_title() override {return "{$SELECTOR_AGAT_INFO}";}
        std::string get_icon() override {return "comment";}
    };

    class ViewerPicAgat : public ViewerPic {
    public:
        ViewerSelectors get_selectors() override;
        ViewerSelectorValues suggest_selectors(const std::string & file_name, const BYTES & data) override;
    protected:
        bool exif_found = false;
        AGAT_EXIF_SECTOR exif;
        int m_palette = 0;
        std::vector<int> m_sizes_to_fit;
        uint32_t convert_color(const int colors, const int palette_id, const int c);
        void start(const BYTES & data, const int frame ) override;
        bool fits(const BYTES & data) override;
    };


}
