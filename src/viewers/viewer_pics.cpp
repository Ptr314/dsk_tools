// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Base class for picture viewers

#include <cstring>
#include "viewer_pics.h"

namespace dsk_tools {
BYTES ViewerPic::process_picture(const BYTES & data, int & sx, int & sy, const int opt)
    {
        start(data, opt);
        sx = get_sx();
        sy = get_sy();

        BYTES imageData(sx * sy * 4); // RGBA
        std::memset(imageData.data(), 0, imageData.size());

        for (int y = 0; y < sy; y++)
            for (int x = 0; x < sx; x++) {
                uint32_t c = get_pixel(x, y);
                int i = (y * sx + x) * 4;
                std::memcpy(imageData.data() + i, &c, sizeof(c));
            }

        return imageData;
    }
}
