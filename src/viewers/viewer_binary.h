// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for binary files

#pragma once

#include "viewer_text.h"

namespace dsk_tools {

    class ViewerBinary : public ViewerText {
    protected:
        virtual std::string make_dump(const BYTES & data, const std::string & cm_name, const unsigned offset);
    public:
        std::string get_type() const override {return "BINARY";}
        std::string get_subtype() const override {return "REGULAR";}
        std::string get_subtype_text() const override {return QT_TRANSLATE_NOOP("viewer", "Regular Binary");}
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
