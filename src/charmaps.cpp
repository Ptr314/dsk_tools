// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Character map selection

#include "charmaps.h"

namespace dsk_tools {

    CharmapInfo init_charmap(const std::string & cm_name)
    {
        CharmapInfo info;

        if (cm_name == "agat") {
            info.charmap = &agat_charmap;
            info.crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "apple2") {
            info.charmap = &apple2_charmap;
            info.crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "apple2c") {
            info.charmap = &apple2c_charmap;
            info.crlf = {0x8d, 0x0D};
        } else
        if (cm_name == "ascii") {
            info.charmap = &ascii_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "koi7_n0_n1") {
            info.charmap = &koi7_n0_n1_charmap;
            info.crlf = {0x8D, 0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
            info.is_koi7 = true;
        } else
        if (cm_name == "koi7_n2") {
            info.charmap = &koi7_n2_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "koi8_r") {
            info.charmap = &koi8_r_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "koi8_m") {
            info.charmap = &koi8_m_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "cp866") {
            info.charmap = &cp866_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "cp1251") {
            info.charmap = &cp1251_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else
        if (cm_name == "iso8859_5") {
            info.charmap = &iso8859_5_charmap;
            info.crlf = {0x0D};
            info.ignore = {0x0A};
            info.txt_end = {0x1A};
            info.tab = 8;
        } else {
            // TODO: Other encodings;
        }

        return info;
    }

}