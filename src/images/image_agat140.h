// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Apple II / Agat 140 Kb FDD images
#pragma once

#include "disk_image.h"

namespace dsk_tools {

    class imageAgat140: public diskImage
    {
        protected:

        public:
            explicit imageAgat140(std::unique_ptr<Loader> loader);
            // 35 to 40 tracks, as many as the loader gives: some disks use the five a
            // drive's head reaches past the 35 that DOS 3.3 formats
            Result load() override;
    };
}
