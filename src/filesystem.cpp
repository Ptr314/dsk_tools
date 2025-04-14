// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for different filesystems

#include "filesystem.h"

namespace dsk_tools {
    fileSystem::fileSystem(diskImage * image):
        image(image)
    {}

    std::string fileSystem::get_delimiter()
    {
        return "\\";
    }

}
