// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - image part
#pragma once

#include "disk_image.h"

namespace dsk_tools {

    class imageFIL: public diskImage
    {
    public:
        explicit imageFIL(std::unique_ptr<Loader> loader);
        Result load() override;
        Result check() override;
    };
}
