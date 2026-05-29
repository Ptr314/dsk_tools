// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for text files

#include "viewer_text.h"
#include "utils.h"

namespace dsk_tools {

    std::string ViewerText::process_as_text(const BYTES &data, const std::string &cm_name)
    {
        cm = init_charmap(cm_name);

        uint8_t koi7_high = 0x80;

        std::string out;

        int last_size = 0;

        for (int a=0; a < data.size(); a++) {
            uint8_t c = data[a];
            if (cm.ignore.find(c) == cm.ignore.end()) {
                if (cm.crlf.find(c) != cm.crlf.end()) {
                    out += "\r";
                    last_size = out.size();
                } else
                    if (cm.tab > 0 && c == 0x09) {
                        int line_size = out.size() - last_size;
                        for (int i=0; i< cm.tab - line_size % cm.tab; i++) out += " ";
                    } else
                        if (cm.txt_end.find(c) != cm.txt_end.end()) {
                            break;
                        } else
                            if (cm.is_koi7) {
                                if (c == 0x0F || c == 0x8F) koi7_high = 0;
                                else
                                if (c == 0x0E || c == 0x8E) koi7_high = 0x80;
                                else
                                    out += (*cm.charmap)[(c & 0x7F) | koi7_high];
                            } else
                                out += (*cm.charmap)[c];
            }
        }
        return escapeHtml(out);
    }

}