// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Direct converters base class
#pragma once

#include <string>

#include "definitions.h"

namespace dsk_tools
{
    class Converter
    {
    public:
        Converter() = default;
        virtual ~Converter() = default;
        virtual Result convert(const std::string & file_in, const std::string & file_out, std::string & log, bool verbose);
        virtual Result convert(const BYTES & in, BYTES & out, std::string & log, bool verbose) = 0;
    };

}
