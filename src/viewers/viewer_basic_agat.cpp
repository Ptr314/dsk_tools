// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat BASIC

#include "viewer_basic_agat.h"
#include "bas_tokens.h"

namespace dsk_tools {

    std::string ViewerBASIC_Agat::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, Agat_tokens, true);
    }

}
