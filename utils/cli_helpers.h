// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: FDD image conversion utility command-line tool

#pragma once

#include "dsk_tools/dsk_tools.h"

namespace dsk_tools {
    enum class CLICommand {none, ls, add, del};
    void setupConsole();
    std::string decode_error(const Result& result);
    Result write_output_file(const std::string & output_file, const std::string & format_id, diskImage * image, const bool verbose);
}
