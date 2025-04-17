// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .DSK files

#ifndef LOADER_RAW_H
#define LOADER_RAW_H

#include "loader.h"

namespace dsk_tools {

    class LoaderRAW:public Loader
    {
        public:
            LoaderRAW(const std::string & file_name, const std::string & format_id, const std::string & type_id);
            virtual int load(BYTES & buffer) override;
            virtual std::string file_info() override;

    };

}

#endif // LOADER_RAW_H
