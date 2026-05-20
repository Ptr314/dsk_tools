// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Vector-06C BASIC

#pragma once

#include "viewer_basic_agat.h"

namespace dsk_tools {

    class ViewerBASIC_Vector : public ViewerBASIC_Agat {
    public:
        static ViewerRegistrar<ViewerBASIC_Vector> registrar;

        std::string get_type() const override {return "BASIC";}
        std::string get_subtype() const override {return "VECTOR";}
        std::string get_subtype_text() const override {return QT_TRANSLATE_NOOP("viewer", "Vector-06C BASIC");}
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
