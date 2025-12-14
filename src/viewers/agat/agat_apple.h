// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat Apple-format picture viewers

#pragma once

#include "agat_base.h"

namespace dsk_tools {

    #define APPLE_HIRES_AGAT_SELECTOR_ID "apple_hires_agat"

    class ViewerSelectorAgatHiresAgat: public ViewerSelector {
        std::string get_id() override {return APPLE_HIRES_AGAT_SELECTOR_ID;}
        std::string get_title() override {return "{$SELECTOR_APPLE_HIRES_AGAT}";}
        std::string get_icon() override {return "palette";}
        bool has_customs() override {return false;}
        ViewerSelectorOptions get_options() override
        {
            ViewerSelectorOptions options;
            options.push_back({"color", "{$COLOR}", ""});
            options.push_back({"mono", "{$MONOCHROME}", ""});
            options.push_back({"custom", "{$CUSTOM_PALETTE}", ""});
            return options;
        };
    };

    #define APPLE_HIRES_APPLE_SELECTOR_ID "apple_hires_apple"

    class ViewerSelectorAgatHiresApple: public ViewerSelector {
        std::string get_id() override {return APPLE_HIRES_APPLE_SELECTOR_ID;}
        std::string get_title() override {return "{$SELECTOR_APPLE_HIRES_APPLE}";}
        std::string get_icon() override {return "palette";}
        bool has_customs() override {return false;}
        ViewerSelectorOptions get_options() override
        {
            ViewerSelectorOptions options;
            options.push_back({"ntsc_imp", "{$NTSC_APPLE_IMPROVED}", ""});
            options.push_back({"ntsc_orig", "{$NTSC_APPLE_ORIGINAL}", ""});
            options.push_back({"bw", "{$BW}", ""});
            return options;
        };
    };

    class ViewerPicAgatApple : public ViewerPicAgat {
    protected:
        uint32_t line_data[560];
        int current_line = -1;
        int m_mode = 0;
        virtual void start(const BYTES & data, const int frame = 0) override;
        virtual void process_line(int line_offset, int y = 0) = 0;
    public:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_280x192HiRes_Agat : public ViewerPicAgatApple {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_280x192HiRes_Agat> registrar;

        ViewerPicAgat_280x192HiRes_Agat() {m_sx = 280; m_sy = 192; m_sizes_to_fit = {8192, 8192+256};}

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "280x192HiRes";}
        std::string get_subtype_text() const override {return "280x192 HGR";}

        ViewerSelectors get_selectors() override;
    };

    class ViewerPicAgat_280x192HiRes_Apple : public ViewerPicAgatApple {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_280x192HiRes_Apple> registrar;

        ViewerPicAgat_280x192HiRes_Apple() {m_sx = 280; m_sy = 192; m_sizes_to_fit = {8192, 8192+256};}

        std::string get_type() const override {return "PICTURE_APPLE";}
        std::string get_subtype() const override {return "280x192HiRes";}
        std::string get_subtype_text() const override {return "280x192 HiRes";}

        ViewerSelectors get_selectors() override;
    };

    class ViewerPicAgat_140x192DblHiRes : public ViewerPicAgatApple {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_140x192DblHiRes> registrar;

        ViewerPicAgat_140x192DblHiRes() {m_sx = 140; m_sy = 192; m_sizes_to_fit = {16384, 16384+256};}

        std::string get_type() const override {return "PICTURE_APPLE";}
        std::string get_subtype() const override {return "140x192DblHiRes";}
        std::string get_subtype_text() const override {return "140x192 Dbl HiRes Color";}
        ViewerSelectors get_selectors() override;
    };

    class ViewerPicAgat_560x192DblHiResBW : public ViewerPicAgatApple {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_560x192DblHiResBW> registrar;

        ViewerPicAgat_560x192DblHiResBW() {m_sx = 560; m_sy = 192; m_sizes_to_fit = {16384, 16384+256};}

        std::string get_type() const override {return "PICTURE_APPLE";}
        std::string get_subtype() const override {return "560x192DblHiResBW";}
        std::string get_subtype_text() const override {return "560x192 Dbl HiRes B/W";}
        ViewerSelectors get_selectors() override;

    };

    class ViewerPicAgat_40x48LoRes : public ViewerPicAgatApple {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_40x48LoRes> registrar;

        ViewerPicAgat_40x48LoRes() {m_sx = 40; m_sy = 48; m_sizes_to_fit = {1280, 1280+256};}

        std::string get_type() const override {return "PICTURE_APPLE";}
        std::string get_subtype() const override {return "40x48LoRes";}
        std::string get_subtype_text() const override {return "40x48 LoRes";}
        ViewerSelectors get_selectors() override;
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_80x48DblLoRes : public ViewerPicAgat_40x48LoRes {
    protected:
        void process_line(int line_offset, int y = 0) override;
    public:
        static ViewerRegistrar<ViewerPicAgat_80x48DblLoRes> registrar;

        ViewerPicAgat_80x48DblLoRes() {m_sx = 80; m_sy = 48; m_sizes_to_fit = {2048, 2048+256};}

        std::string get_type() const override {return "PICTURE_APPLE";}
        std::string get_subtype() const override {return "80x48DblLoRes";}
        std::string get_subtype_text() const override {return "80x48 Dbl LoRes";}
    };

}
