// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - loader part

#ifndef LOADER_FIL_H
#define LOADER_FIL_H

#include "loader.h"

namespace dsk_tools {

    class LoaderFIL:public Loader
    {
    public:
        LoaderFIL(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual int load(BYTES & buffer) override;
        virtual std::string file_info() override;
    };

}

#endif // LOADER_FIL_H
