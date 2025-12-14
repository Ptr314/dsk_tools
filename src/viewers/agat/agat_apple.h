// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Agat Apple-format picture viewers

#pragma once

#include "agat_base.h"

namespace dsk_tools {

    class ViewerPicAgatApple : public ViewerPicAgat {
    protected:
        uint32_t line_data[560];
        int current_line = -1;
        virtual void start(const BYTES & data, const int opt, const int frame = 0) override;
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

        PicOptions get_options() override;
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

        PicOptions get_options() override;
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
        PicOptions get_options() override;
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
        PicOptions get_options() override;
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
        PicOptions get_options() override;
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
