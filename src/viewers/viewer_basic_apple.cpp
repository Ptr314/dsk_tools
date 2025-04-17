// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Applesoft BASIC

#include "viewer_basic_apple.h"
#include "bas_tokens.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_Apple> ViewerBASIC_Apple::registrar;

    std::string ViewerBASIC_Apple::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, dsk_tools::ABS_tokens);
    }
}
