// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for tokenized BBC BASIC, as used by the Onix OS of the Agat

#pragma once

#include "viewer.h"
#include "viewers/viewer_basic.h"

namespace dsk_tools {

    class ViewerBASIC_BBC : public ViewerBASIC {
    public:
        std::string get_type() const override {return "BASIC";}
        std::string get_subtype() const override {return "BBC";}
        std::string get_subtype_text() const override {return QT_TRANSLATE_NOOP("viewer", "BBC BASIC");}
        bool fits(const BYTES & data, const std::string & file_name) override;
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
