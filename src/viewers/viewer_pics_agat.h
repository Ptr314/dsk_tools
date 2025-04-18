// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewers for Agat pictures

#pragma once

#include "viewer_pics.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    // https://agatcomp.ru/agat/PCutils/EXIF.shtml
    struct AGAT_EXIF_SECTOR {
        uint8_t     _NOT_USED_0[4];
        uint8_t     SIGNATURE[2];           // Must be "VR" ($D6 $D2)
        uint8_t     MODE;                   // Mode
        uint8_t     PALETTE;                // Palette id
        uint8_t     R[8];                   // Palette R
        uint8_t     _NOT_USED_1[8];
        uint8_t     G[8];                   // Palette R
        uint8_t     _NOT_USED_2[8];
        uint8_t     B[8];                   // Palette B
        uint8_t     CHARSET_NAME[15];       // Charset id (if FONT==$Fx below)
        uint8_t     FONT;                   // Font id ($Fx for charset above)
        uint8_t     COMMENT[12*16];
    };

    #pragma pack(pop)

    static const uint8_t Agat_16_palette[16][3]  = {
        {  0,   0,   0}, {217,   0,   0}, {  0, 217,   0}, {217, 217,   0},
        {  0,   0, 217}, {217,   0, 217}, {  0, 217, 217}, {217, 217, 217},
        { 38,  38, 	38}, {255,  38,  38}, { 38, 255,  38}, {255, 255,  38},
        { 38,  38, 255}, {255,  38, 255}, { 38, 255, 255}, {255, 255, 255}
    };

    static const uint8_t Agat_4_palette[4][4][3]  = {
        //    00                01                10               11
        {{  0,   0,   0}, {255,   0,   0}, {  0, 255,   0}, {  0,   0, 255}},    // Palette 1
        {{255, 255, 255}, {255,   0,   0}, {  0, 255,   0}, {  0,   0, 255}},    // Palette 2
        {{  0,   0,   0}, {  0,   0,   0}, {  0, 255,   0}, {  0,   0, 255}},    // Palette 3
        {{  0,   0,   0}, {255,   0,   0}, {  0,   0,   0}, {  0,   0, 255}}     // Palette 4
    };


    static const uint8_t Agat_2_palette[4][2][3] =  {
        //       0               1
        {{  0,   0,   0}, {255, 255, 255}},     // Palette 1
        {{255, 255, 255}, {  0,   0,   0}},     // Palette 2
        {{  0,   0,   0}, {  0, 255,   0}},     // Palette 3
        {{  0, 255,   0}, {  0,   0,   0}}      // Palette 4
    };

    class ViewerPicAgat : public ViewerPic {
    public:
        PicOptions get_options() override;
    protected:
        bool exif_found = false;
        AGAT_EXIF_SECTOR exif;
        int m_palette = 0;
        uint32_t convert_color(const int colors, const int palette, const int c);
        virtual void start(const BYTES & data, const int opt) override;
    };

    class ViewerPicAgat16 : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_64x64x16 : public ViewerPicAgat16 {
    public:
        static ViewerRegistrar<ViewerPicAgat_64x64x16> registrar;

        ViewerPicAgat_64x64x16() {m_sx = 64; m_sy = 64;};

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "64x64x16";}
        std::string get_subtype_text() const override {return "64x64 ЦГНР";}
    };

    class ViewerPicAgat_128x128x16 : public ViewerPicAgat16 {
    public:
        static ViewerRegistrar<ViewerPicAgat_128x128x16> registrar;

        ViewerPicAgat_128x128x16() {m_sx = 128; m_sy = 128;};

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "128x128x16";}
        std::string get_subtype_text() const override {return "128x128 ЦГСР";}
    };

    class ViewerPicAgat4 : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_256x256x4 : public ViewerPicAgat4 {
    public:
        static ViewerRegistrar<ViewerPicAgat_256x256x4> registrar;

        ViewerPicAgat_256x256x4() {m_sx = 256; m_sy = 256;};

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "256x256x4";}
        std::string get_subtype_text() const override {return "256х256 ЦГВР";}
    };

    class ViewerPicAgatMono : public ViewerPicAgat {
    protected:
        uint32_t get_pixel(int x, int y) override;
    };

    class ViewerPicAgat_256x256x1 : public ViewerPicAgatMono {
    public:
        static ViewerRegistrar<ViewerPicAgat_256x256x1> registrar;

        ViewerPicAgat_256x256x1() {m_sx = 256; m_sy = 256;};

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "256x256x1";}
        std::string get_subtype_text() const override {return "256х256 МГВР";}
    };

    class ViewerPicAgat_512x256x1 : public ViewerPicAgatMono {
    public:
        static ViewerRegistrar<ViewerPicAgat_512x256x1> registrar;

        ViewerPicAgat_512x256x1() {m_sx = 512; m_sy = 256;};

        std::string get_type() const override {return "PICTURE_AGAT";}
        std::string get_subtype() const override {return "512x256x1";}
        std::string get_subtype_text() const override {return "512х256 МГДП";}
    };


}
