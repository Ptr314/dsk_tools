// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Top level abstract class for all loaders

#include "loader.h"

namespace dsk_tools {
    Loader::Loader(const std::string & file_name, const std::string & format_id, const std::string & type_id):
          file_name(file_name)
        , format_id(format_id)
        , type_id(type_id)
        , loaded(false)
    {}

}
