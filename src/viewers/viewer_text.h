// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for text files

#pragma once

#include "viewer.h"

namespace dsk_tools {

    class ViewerText : public Viewer {
    public:
        static ViewerRegistrar<ViewerText> registrar;

        std::string get_type() const override {return "TEXT";}
        std::string get_subtype() const override {return "";}
        std::string get_subtype_text() const override {return "";}
        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;

    protected:
        const std::string (*charmap)[256];
        std::set<uint8_t> crlf;
        std::set<uint8_t> ignore = {};
        std::set<uint8_t> txt_end = {};
        int tab = 0;
        virtual void init_charmap(std::string cm_name);

    };

}
