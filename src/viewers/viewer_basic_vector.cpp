// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Vector-06C BASIC

#include "viewer_basic_vector.h"
#include "bas_tokens.h"

namespace dsk_tools {

    std::string ViewerBASIC_Vector::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, Vector_tokens, false);
    }
}
