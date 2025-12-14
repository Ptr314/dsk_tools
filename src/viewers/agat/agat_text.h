// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat text-mode picture viewers

#pragma once

#include <vector>
#include "agat_base.h"

namespace dsk_tools {

    #define AGAT_FONT_SELECTOR_ID "agat_font_type"

    class ViewerSelectorAgatFont: public ViewerSelector {
        std::string get_id() override {return AGAT_FONT_SELECTOR_ID;}
        std::string get_title() override {return "{$SELECTOR_AGAT_FONT}";}
        std::string get_icon() override {return "font";}
        bool has_customs() override {return false;}
        ViewerSelectorOptions get_options() override
        {
            ViewerSelectorOptions options;
            options.push_back({"a7_classic", "{$AGAT_FONT_A7_CLASSIC}", ""});
            options.push_back({"a7_enhanced", "{$AGAT_FONT_A7_ENCHANCED}", ""});
            options.push_back({"a9_classic", "{$AGAT_FONT_A9_CLASSIC}", ""});
            options.push_back({"loaded", "{$AGAT_FONT_CUSTOM_LOADED}", ""});
            return options;
        };
    };

    // Agat T32 color & T64 bw superclass
    class ViewerPicAgatText : public ViewerPicAgat {
    protected:
        uint32_t line_data[512];
        int current_line = -1;
        const uint8_t (*m_font)[2048];
        bool m_font_reverse = false;
        uint8_t m_custom_font[2048];
        bool m_custom_reverse;
        virtual void start(const BYTES & data, const int frame = 0) override;
        virtual void process_line(int y = 0) = 0;
        virtual bool load_custom_font();
    public:
        uint32_t get_pixel(int x, int y) override;
        int prepare_data(const BYTES & data, dsk_tools::diskImage & image, dsk_tools::fileSystem & filesystem, std::string & error_msg) override;

        ViewerSelectors get_selectors() override;
        ViewerSelectorValues suggest_selectors(const std::string file_name, const BYTES & data) override;
    };

    class ViewerPicAgatTextT32 : public ViewerPicAgatText {
    protected:
        void process_line(int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgatTextT32> registrar;

        ViewerPicAgatTextT32() {m_sx = 256; m_sy = 256; m_sizes_to_fit = {2048, 2048+256};}
        int get_frame_delay() const override {return 1000;}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "T32";}
        std::string get_subtype_text() const override {return "T32";}
    };

    class ViewerPicAgatTextT64 : public ViewerPicAgatText {
    protected:
        void process_line(int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgatTextT64> registrar;

        ViewerPicAgatTextT64() {m_sx = 512; m_sy = 256; m_sizes_to_fit = {2048, 2048+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "T64";}
        std::string get_subtype_text() const override {return "T64";}
    };

}
