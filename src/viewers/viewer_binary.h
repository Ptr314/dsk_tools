// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for binary files

#pragma once

#include "viewer_text.h"

namespace dsk_tools {

    class ViewerBinary : public ViewerText {
    public:
        static ViewerRegistrar<ViewerBinary> registrar;

        std::string get_type() const override {return "BINARY";}
        std::string get_subtype() const override {return "";}
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
