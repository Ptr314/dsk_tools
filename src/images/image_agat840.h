// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Agat 840 Kb FDD images
#pragma once

#define IMAGE_AGAT840_H

#include "image_agat140.h"

namespace dsk_tools {

    class imageAgat840: public imageAgat140
    {
    public:
        imageAgat840(std::unique_ptr<Loader> loader);
        uint8_t *get_sector_data(unsigned head, unsigned track, unsigned sector) override;
    };

}
