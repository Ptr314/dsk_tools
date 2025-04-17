// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for text files

#include "viewer_text.h"
#include "charmaps.h"


namespace dsk_tools {
    ViewerRegistrar<ViewerText> ViewerText::registrar;

    void ViewerText::init_charmap(std::string cm_name)
    {
        if (cm_name == "agat") {
            charmap = &dsk_tools::agat_charmap;
            crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "apple2") {
            charmap = &dsk_tools::apple2_charmap;
            crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "apple2c") {
            charmap = &dsk_tools::apple2c_charmap;
            crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "ascii") {
            charmap = &dsk_tools::ascii_charmap;
            crlf = {0x0D};
            ignore = {0x0A};
            txt_end = {0x1A};
            tab = 8;
        } else {
            // TODO: Other encodings;
        }
    }

    std::string ViewerText::process_as_text(const BYTES &data, const std::string &cm_name)
    {
        init_charmap(cm_name);

        std::string out;

        int last_size = 0;

        for (int a=0; a < data.size(); a++) {
            uint8_t c = data[a];
            if (ignore.find(c) == ignore.end()) {
                if (crlf.find(c) != crlf.end()) {
                    out += "\r";
                    last_size = out.size();
                } else
                    if (tab > 0 && c == 0x09) {
                        int line_size = out.size() - last_size;
                        for (int i=0; i< tab - line_size % tab; i++) out += " ";
                    } else
                        if (txt_end.find(c) != txt_end.end()) {
                            break;
                        } else
                            out += (*charmap)[c];
            }
        }
        return out;
    }

}
