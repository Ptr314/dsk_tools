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

    class ViewerPicAgat : public ViewerPic {
    public:
        PicOptions get_options() override;
        int suggest_option(const std::string file_name, const BYTES & data) override;
    protected:
        bool exif_found = false;
        AGAT_EXIF_SECTOR exif;
        int m_palette = 0;
        std::vector<int> m_sizes_to_fit;
        uint32_t convert_color(const int colors, const int palette_id, const int c);
        void start(const BYTES & data, const int opt, const int frame ) override;
        bool fits(const BYTES & data) override;
    };


}
