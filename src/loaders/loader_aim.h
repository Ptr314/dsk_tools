// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for AIM (Agat 840 Kb psysical images)
#pragma once


#include "loader.h"

namespace dsk_tools {

    class LoaderAIM:public Loader
    {
    private:
        bool iterate_until(const std::vector<uint16_t> &in, int &p, const uint8_t v);
    protected:
        bool msb_first;
    public:
        LoaderAIM(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual ~LoaderAIM() = default;
        Result load(std::vector<uint8_t> &buffer) override;
        std::string file_info() override;
    };

}
