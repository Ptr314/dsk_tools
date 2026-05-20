// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for BASIC base class

#pragma once

#include "viewer.h"
#include "viewers/viewer_text.h"

namespace dsk_tools {

    enum class EntityType { NONE, LINE_NUMBER, TOKEN, VAR, CHAR, STRING, NUMBER, BR, REM, ASM, ASM_LABEL };

    std::string entityTypeToString(EntityType type);

    class ViewerBASIC : public ViewerText {

    protected:
        std::string convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens, const bool use_globals);
    };

}
