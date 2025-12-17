// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat text-mode picture viewers implementation

#include <cstring>
#include <algorithm>
#include "agat_text.h"
#include "agat_fonts.h"
#include "fs_dos33.h"
#include "loader_fil.h"
#include "utils.h"

namespace dsk_tools {

    void ViewerPicAgatText::start(const BYTES & data, const int frame)
    {
        current_line = -1;
        ViewerPicAgat::start(data, frame);

        const std::string font = m_selectors[AGAT_FONT_SELECTOR_ID];
        const std::string pal_id = m_selectors[AGAT_PALETTE_SELECTOR_ID];
        if (!pal_id.empty()) {
            // Skip overwriting m_palette if it's a custom palette file (handled by parent)
            if (pal_id.size() <= 7 || pal_id.substr(0, 7) != "custom:") {
                try {
                    m_palette = std::stoi(pal_id);
                } catch (...) {
                    m_palette = 0;
                }
            }
        }

        if (font == "a7_classic") {
            m_font = &A7_font;
            m_font_reverse = false;
        } else
        if (font == "a7_enhanced") {
            m_font = &A7_font_svt;
            m_font_reverse = false;
        } else
        if (font == "a9_classic") {
            m_font = &A9_font;
            m_font_reverse = true;
        } else
        if (font == "loaded") {
            m_font = &m_custom_font;
            m_font_reverse = m_custom_reverse;
        } else {
            if (font.size() > 7 && font.substr(0, 7) == "custom:") {
                const std::string file_name = font.substr(7);
                if (m_external_font_file != file_name) {
                    std::memset(&m_external_font, 0xAA, sizeof(m_external_font));
                    BYTES buffer;
                    if (file_exists(file_name)) {
                        LoaderFIL loader(file_name, "", "");
                        const auto load_res = loader.load(buffer);
                        if (load_res) {
                            const auto * header = reinterpret_cast<FIL_header*>(buffer.data());
                            std::string internal_name = to_upper(trim(agat_to_utf(header->name, 30)));
                            if (internal_name.substr(0, 2) == "ZG") {
                                m_external_reverse = internal_name.substr(2, 1) == "9";
                                std::memcpy(&m_external_font, buffer.data()+44, 2048);
                            }
                        }
                    }
                    m_external_font_file = file_name;
                }
                m_font = &m_external_font;
                m_font_reverse = m_external_reverse;
            } else {
                m_font = &A7_font;
                m_font_reverse = false;
            }
        }
    }

    Result ViewerPicAgatText::prepare_data(const BYTES & data, dsk_tools::diskImage & image, dsk_tools::fileSystem & filesystem, std::string & error_msg)
    {
        error_msg = "";
        std::memset(&m_custom_font, 0xAA, sizeof(m_custom_font));

        auto res = ViewerPicAgat::prepare_data(data, image, filesystem, error_msg);
        if (!res) return res;

        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                int font = exif.FONT >> 4;
                if (font == 0xF) {
                    std::string font_name = to_upper(trim(agat_to_utf(exif.FONT_NAME, 15)));

                    BYTES buffer;
                    std::string file_name;

                    // Trying to find on a loaded filesystem
                    UniversalFile fd;
                    file_name = "ZG9_"+font_name;
                    if (filesystem.find_file(file_name,fd)) {
                        filesystem.get_file(fd, "", buffer);
                        m_custom_reverse = true;
                        std::memcpy(&m_custom_font, buffer.data()+4, 2048);
                        return Result::ok();
                    }
                    file_name = "ZG7_"+font_name;
                    if (filesystem.find_file(file_name,fd)) {
                        filesystem.get_file(fd, "", buffer);
                        m_custom_reverse = false;
                        std::memcpy(&m_custom_font, buffer.data()+4, 2048);
                        return Result::ok();
                    }

                    // Trying to find on a host
                    std::string file_path = get_file_path(image.file_name());
                    file_name = file_path + font_name + ".fil";
                    if (file_exists(file_name)) {
                        LoaderFIL loader(file_name, "", "");
                        const auto load_res = loader.load(buffer);
                        if (load_res) {
                            auto * header = reinterpret_cast<FIL_header*>(buffer.data());
                            std::string internal_name = to_upper(trim(agat_to_utf(header->name, 30)));
                            if (internal_name.substr(0, 2) == "ZG") {
                                m_custom_reverse = internal_name.substr(2, 1) == "9";
                                std::memcpy(&m_custom_font, buffer.data()+44, 2048);
                                return Result::ok();
                            }
                        } else {
                            return Result::error(ErrorCode::PreparePicError);
                        }
                    }
                    error_msg = "{$FONT_LOADING_ERROR}: " + font_name;
                    return Result::error(ErrorCode::PreparePicError);
                }
            }
        }
        return Result::ok();
    }

    bool ViewerPicAgatText::load_custom_font()
    {
        std::string font_name = to_upper(trim(agat_to_utf(exif.FONT_NAME, 15)));
        return false;
    }

    uint32_t ViewerPicAgatText::get_pixel(int x, int y)
    {
        if (current_line != y) {
            process_line(y);
            current_line = y;
        }
        return line_data[x];
    }

    ViewerSelectors ViewerPicAgatText::get_selectors()
    {
        std::vector<std::unique_ptr<ViewerSelector>> result;
        result.push_back(make_unique<ViewerSelectorAgatFont>());
        result.push_back(make_unique<ViewerSelectorAgatPalette>());
        result.push_back(make_unique<ViewerSelectorAgatInfo>());
        return result;
    }

    ViewerSelectorValues ViewerPicAgatText::suggest_selectors(const std::string & file_name, const BYTES & data)
    {
        ViewerSelectorValues result;
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            std::memcpy(&exif, data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR), sizeof(AGAT_EXIF_SECTOR));
            if (exif.SIGNATURE[0] == 0xD6 && exif.SIGNATURE[1] == 0xD2) {
                exif_found = true;
                const int font = exif.FONT >> 4;
                if (font == 7) result[AGAT_FONT_SELECTOR_ID] = "a7_classic";
                if (font == 8) result[AGAT_FONT_SELECTOR_ID] = "a7_enhanced";
                if (font == 9) result[AGAT_FONT_SELECTOR_ID] = "a9_classic";
                if (font == 0xF) result[AGAT_FONT_SELECTOR_ID] = "loaded";

                result[AGAT_PALETTE_SELECTOR_ID] = std::to_string(exif.PALETTE >> 4);
            }
        }
        return result;
    }


    void ViewerPicAgatTextT32::process_line(int y)
    {
        static uint32_t black = 0xFF000000;

        // 32 pixels blanking of sides
        for (int i=0; i<16; i++) {
            line_data[i] = black;
            line_data[i+240] = black;
        }

        bool blinker = m_frame & 1;

        int file_offset = 4;

        int line_num = y >> 3;
        int font_lin = y & 7;
        for (int char_num = 0; char_num < 32; char_num++) {
            int index = file_offset + line_num*64 + char_num*2;
            if (index + 1 < m_data->size()) {
                uint8_t v1 = m_data->at(index);
                uint8_t v2 = m_data->at(index+1);
                uint8_t cl = (v2 & 0x07) | ((v2 & 0x10) >> 1);
                for (int k = 0; k < 7; k++) {
                    uint8_t font_val = (*m_font)[v1*8+font_lin];
                    if (!m_font_reverse)
                        font_val = ~font_val;
                    uint8_t c = ((m_font_reverse)?(font_val >> (7-k)):(font_val >> k)) & 0x01;
                    uint8_t ccl;
                    if ( (((v2 & 0x20) != 0) || ((v2 & 0x08) != 0) && blinker) )
                        ccl = cl * c;
                    else
                        ccl = cl * (c ^ 0x01);
                    uint32_t color = convert_color(16, m_palette, ccl);
                    int pi = char_num*7 + k;
                    line_data[16 + pi] = color;
                }
            }
        }
    }

    void ViewerPicAgatTextT64::process_line(int y)
    {
        static uint32_t black = 0xFF000000;

        // 32 pixels blanking of sides
        for (int i=0; i<32; i++) {
            line_data[i] = black;
            line_data[i+480] = black;
        }

        const int line_num = y >> 3;
        const int font_lin = y & 7;
        for (int char_num = 0; char_num < 64; char_num++) {
            constexpr int file_offset = 4;
            const int index = file_offset + line_num*64 + char_num;
            if (index < m_data->size()) {
                const uint8_t v1 = m_data->at(index);
                for (int k = 0; k < 7; k++) {
                    uint8_t font_val = (*m_font)[v1*8+font_lin] ;
                    if (!m_font_reverse)
                        font_val = ~font_val;
                    const uint8_t c = (((m_font_reverse)?(font_val >> (7-k)):(font_val >> k)) & 0x01);
                    const uint32_t color = convert_color(2, m_palette, c);
                    const int pi = char_num*7 + k;
                    line_data[32 + pi] = color;
                }
            }
        }
    }

    // Static registrar instantiations
    ViewerRegistrar<ViewerPicAgatTextT32> ViewerPicAgatTextT32::registrar;
    ViewerRegistrar<ViewerPicAgatTextT64> ViewerPicAgatTextT64::registrar;

}
