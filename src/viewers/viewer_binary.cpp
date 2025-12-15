// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for binary files

#include "viewer_binary.h"
#include "utils.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBinary> ViewerBinary::registrar;

std::string ViewerBinary::process_as_text(const BYTES & data, const std::string & cm_name) {
        std::string out;

        init_charmap(cm_name);

        std::string text = "    ";
        for (int a=0; a < data.size(); a++) {
            if (a % 16 == 0)  {
                if (a != 0) out += text + "\n";
                out += int_to_hex(static_cast<uint16_t>(a)) + " ";
                text = "    ";
            }
            out += " " + int_to_hex(static_cast<uint8_t>(data[a]));
            text += (*charmap)[data[a]];
        }
        out += text;

        return escapeHtml(out);
    }

}

