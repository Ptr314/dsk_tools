// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Agat 880 Kb FDD images (11 sectors of 512 bytes, Nippel OS)
#pragma once

#include "disk_image.h"

namespace dsk_tools {

    class imageAgat880: public diskImage
    {
    public:
        explicit imageAgat880(std::unique_ptr<Loader> loader);
        uint8_t *get_sector_data(unsigned head, unsigned track, unsigned sector) override;
    };

}
