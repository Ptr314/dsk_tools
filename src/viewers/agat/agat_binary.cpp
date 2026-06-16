// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat binary files

#include "agat_binary.h"
#include "utils.h"

namespace dsk_tools {

    std::string ViewerBinaryAgat::process_as_text(const BYTES & data, const std::string & cm_name) {
        if (data.size()<4) return "";
        uint16_t a = data[0] + (data[1]<<8);
        uint16_t l = data[2] + (data[3]<<8);
        size_t end = std::min(static_cast<size_t>(4 + l), data.size());
        BYTES payload(data.begin() + 4, data.begin() + end);
        return make_dump(payload, cm_name, a);
    }

}

