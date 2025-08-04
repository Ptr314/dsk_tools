// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat BASIC

#pragma once

#include "viewer.h"
#include "viewers/viewer_text.h"

namespace dsk_tools {

    enum class EntityType { NONE, LINE_NUMBER, TOKEN, VAR, CHAR, STRING, NUMBER, BR, REM, ASM, ASM_LABEL };

    std::string entityTypeToString(EntityType type);

    class ViewerBASIC_Agat : public ViewerText {
    public:
        static ViewerRegistrar<ViewerBASIC_Agat> registrar;

        std::string get_type() const override {return "BASIC";}
        std::string get_subtype() const override {return "AGAT";}
        virtual std::string get_subtype_text() const override {return "Agat BASIC";}
        std::string process_as_text(const BYTES &data, const std::string &cm_name) override;

    protected:
        std::string convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens);
    };

}
