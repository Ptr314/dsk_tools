// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .HFE files
#pragma once


#include "loader.h"

namespace dsk_tools {

    class LoaderHXC_HFE:public Loader
    {
    public:
        LoaderHXC_HFE(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        Result load(BYTES & buffer) override;
        std::string file_info() override;
    };

}
