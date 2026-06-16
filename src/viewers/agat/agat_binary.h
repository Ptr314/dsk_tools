// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Agat binary files

#pragma once

#include "viewers/viewer_binary.h"

namespace dsk_tools {

    class ViewerBinaryAgat : public ViewerBinary {
    public:
        std::string get_type() const override {return "BINARY";}
        std::string get_subtype() const override {return "AGAT";}
        std::string get_subtype_text() const override {return QT_TRANSLATE_NOOP("viewer", "Agat Executable");}
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
