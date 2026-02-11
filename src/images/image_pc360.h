// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for IBM PC 360 images
#pragma once

#include "disk_image.h"

namespace dsk_tools {

    class imagePC360: public diskImage
    {
    public:
        explicit imagePC360(std::unique_ptr<Loader> loader, bool is_interleaved);
        Result check() override;
    };
}
