// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for Agat binary files

#include "agat_binary.h"
#include "utils.h"

namespace dsk_tools {

    // An Agat DOS binary starts with a load address and a length, and is stored in whole
    // 256 byte sectors. Files of other filesystems (ProDOS of the Nippel OS, CP/M, FAT) carry no
    // such header: reading their first bytes as one shifts the dump by four bytes and
    // labels it with a nonsense address, so the mode is offered only when it adds up.
    bool ViewerBinaryAgat::fits(const BYTES & data, const std::string & file_name)
    {
        if (data.size() < 5) return false;

        const size_t length = data[2] + (data[3] << 8);
        if (length == 0) return false;

        const size_t declared = 4 + length;
        if (declared > data.size()) return false;

        // The file is read whole sectors at a time, and one spare sector past the end
        // of the data is not unusual, so up to two sectors of slack are allowed
        return data.size() - declared < 512;
    }

    std::string ViewerBinaryAgat::process_as_text(const BYTES & data, const std::string & cm_name) {
        // Without a plausible header there is nothing to shift by: show it as it is
        if (!fits(data, "")) return ViewerBinary::process_as_text(data, cm_name);

        uint16_t a = data[0] + (data[1]<<8);
        uint16_t l = data[2] + (data[3]<<8);
        size_t end = std::min(static_cast<size_t>(4 + l), data.size());
        BYTES payload(data.begin() + 4, data.begin() + end);
        return make_dump(payload, cm_name, a);
    }

}

