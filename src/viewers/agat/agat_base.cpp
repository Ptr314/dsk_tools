// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Base Agat picture viewer implementation

#include <cstring>
#include <algorithm>
#include <fstream>
#include "agat_base.h"

#include "utils.h"
#include "host_helpers.h"

namespace dsk_tools {

    ViewerSelectors ViewerPicAgat::get_selectors()
    {
        std::vector<std::unique_ptr<ViewerSelector>> result;
        result.push_back(make_unique<ViewerSelectorAgatPalette>());
        return result;
    }

    ViewerSelectorValues ViewerPicAgat::suggest_selectors(const std::string file_name, const BYTES & data)
    {
        ViewerSelectorValues result;
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                result[AGAT_PALETTE_SELECTOR_ID] = std::to_string(exif.PALETTE >> 4);
            }
        }
        return result;
    }

    void ViewerPicAgat::start(const BYTES & data, const int frame)
    {
        ViewerPic::start(data, frame);

        // Process Agat "EXIF"
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
            }
            const std::string pal_id = m_selectors[AGAT_PALETTE_SELECTOR_ID];
            if (!pal_id.empty()) {
                // Check if this is a custom palette file
                if (pal_id.size() > 7 && pal_id.substr(0, 7) == "custom:") {
                    // Handle custom palette file
                    std::string file_path = pal_id.substr(7);  // Remove "custom:" prefix
                    UTF8_ifstream file(file_path, std::ios::binary);
                    if (file.is_open()) {
                        // Get file size
                        file.seekg(0, std::ios::end);
                        std::streampos file_size = file.tellg();

                        // Read EXIF data from the very end of the file
                        if (file_size > static_cast<std::streampos>(sizeof(AGAT_EXIF_SECTOR))) {
                            file.seekg(-static_cast<std::streamoff>(sizeof(AGAT_EXIF_SECTOR)), std::ios::end);
                            char buffer[sizeof(AGAT_EXIF_SECTOR)];
                            file.read(buffer, sizeof(AGAT_EXIF_SECTOR));

                            // Copy the read data into exif
                            std::memcpy(&exif, buffer, sizeof(AGAT_EXIF_SECTOR));
                            m_palette = 15;  // 0xF - custom palette
                            exif_found = true;
                        }
                        file.close();
                    }
                } else {
                    // Handle numeric palette ID
                    try {
                        m_palette = std::stoi(pal_id);
                    } catch (...) {
                        m_palette = 0;
                    }
                }
            }

        };
    }

    bool ViewerPicAgat::fits(const BYTES & data)
    {
        if (m_sizes_to_fit.size() != 0)
            return std::find(m_sizes_to_fit.begin(), m_sizes_to_fit.end(), data.size()) != m_sizes_to_fit.end();
        else
            return true;
    }

    uint32_t ViewerPicAgat::convert_color(const int colors, const int palette_id, const int c)
    {
        int res = 0xFF000000; // ABGR (little-endian RGBA in memory)

        // https://agatcomp.ru/agat/Hardware/useful/ColorSet.shtml
        // https://agatcomp.ru/agat/PCutils/EXIF.shtml
        // 0-3: standard palette
        // 8-B: grayscale pallette
        //   F: custom palette from EXIF

        // Firstly we choose from standard palettes
        const uint8_t (*palette)[16][3] = &Agat_16_color;
        if (palette_id >=0x8 && palette_id <= 0xB)
            palette = &Agat_16_gray;

        // Each color maps to a 16-color palette (color, grayscale or custom)
        int c16 = c;
        if (palette_id < 0xF) {
            // Standard palettes
            if (colors == 2)
                c16 = Agat_2_index[palette_id & 0x3][c];
            else
            if (colors == 4)
                c16 = Agat_4_index[palette_id & 0x3][c];
            else
            if (colors == 16)
                c16 = c;
            std::memcpy(&res, (*palette)[c16], 3);
        } else {
            // Custom palette from EXIF
            if (colors == 2)
                c16 = Agat_2_index[0][c];
            else
            if (colors == 4)
                c16 = Agat_4_index[0][c];
            else
            if (colors == 16)
                c16 = c;

            // Each palette byte stores two 4-bit values
            int n = c16 / 2;                                // Position in the custom palette
            int shft = (~(c16 & 1) & 1) * 4;                // 0 or 4
            uint8_t R = ((exif.R[n] >> shft) & 0xF) * 17;   // Map 0-F to 00-FF
            uint8_t G = ((exif.G[n] >> shft) & 0xF) * 17;
            uint8_t B = ((exif.B[n] >> shft) & 0xF) * 17;
            res |= (B << 16) | (G << 8) | R;
        }
        return res;
    }

}
