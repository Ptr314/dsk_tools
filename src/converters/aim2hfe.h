// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: AIM to HFE converter

#pragma once
#include <string>

#include "converter.h"
#include "definitions.h"

namespace dsk_tools
{
    class AIM2HFEConverter : public Converter
    {
        public:
        Result convert(const BYTES & in, BYTES & out, std::string & log, bool verbose) override;
    };
}
