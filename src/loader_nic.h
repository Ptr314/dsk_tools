// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .NIC files

#ifndef LOADER_NIC_H
#define LOADER_NIC_H

#include "loader_mfm.h"

namespace dsk_tools {

    class LoaderNIC:public LoaderMFM
    {
    public:
        LoaderNIC(const std::string & file_name, const std::string & format_id, const std::string & type_id);
    };

}

#endif // LOADER_NIC_H
