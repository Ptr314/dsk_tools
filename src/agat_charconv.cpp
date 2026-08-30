// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Conversion between the Agat character set and UTF-8
//
// Kept apart from utils.cpp, so that a tool using only the file helpers
// from there does not link the character maps

#include <string>
#include <vector>

#include "utils.h"
#include "charmaps.h"

namespace dsk_tools
{
    std::string agat_to_utf(const uint8_t in[], int len)
    {
        std::string out;
        for (int i=0; i < len; i ++) {
            out.append(dsk_tools::agat_charmap[in[i]]);
        }

        return out;
    }

    std::vector<uint8_t> utf_to_agat(const std::string& input) {
        auto& map = get_reverse_agat_charmap();

        std::vector<uint8_t> output;

        std::vector<std::string> chars = split_utf8_chars(input);

        for (const auto& ch : chars) {
            auto it = map.find(ch);
            if (it != map.end()) {
                output.push_back(static_cast<uint8_t>(it->second));
            } else {
                // Код символа "?" (в agat_charmap это символ с кодом 175)
                output.push_back(175);  // '?'
            }
        }

        return output;
    }
}
