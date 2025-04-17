// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for Apple II / Agat 140 Kb FDD images

#ifndef IMAGE_AGAT140_H
#define IMAGE_AGAT140_H

#include "disk_image.h"

namespace dsk_tools {

    class imageAgat140: public diskImage
    {
        protected:

        public:
            imageAgat140(Loader * loader);
            virtual int check() override;
    };
}

#endif // IMAGE_AGAT140_H
