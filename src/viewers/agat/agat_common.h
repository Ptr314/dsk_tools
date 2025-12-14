// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Common Agat picture definitions and color arrays

#pragma once

#include <cstdint>

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
        uint8_t     FONT_NAME[15];          // Font id (if FONT==$Fx below)
        uint8_t     FONT;                   // Font id ($Fx for charset above)
        uint8_t     COMMENT[12*16];
    };

    #pragma pack(pop)

    // https://agatcomp.ru/agat/Hardware/useful/ColorSet.shtml
    static const uint8_t Agat_16_color[16][3]  = {
        {  0,   0,   0}, {217,   0,   0}, {  0, 217,   0}, {217, 217,   0},
        {  0,   0, 217}, {217,   0, 217}, {  0, 217, 217}, {217, 217, 217},
        { 38,  38, 	38}, {255,  38,  38}, { 38, 255,  38}, {255, 255,  38},
        { 38,  38, 255}, {255,  38, 255}, { 38, 255, 255}, {255, 255, 255}
    };

    static const uint8_t Agat_16_gray[16][3]  = {
        {  0,   0,   0}, {130, 130, 130}, { 89,  89,  89}, {221, 221, 221},
        { 65,  65,  65}, {194, 194, 194}, {151, 151, 151}, {241, 241, 241},
        { 39,  39, 	39}, {185, 185, 185}, {148, 148, 148}, {244, 244, 244},
        {108, 108, 108}, {229, 229, 229}, {197, 197, 197}, {255, 255, 255}
    };

    static const uint8_t Agat_4_index[4][4]  = {
      // 0    1   2   3
        { 0,  1,  2,  4},    // Palette 1
        {15,  1,  2,  4},    // Palette 2
        { 0,  0,  2,  4},    // Palette 3
        { 0,  1,  0,  4}     // Palette 4
    };

    static const uint8_t Agat_2_index[4][2] =  {
      //  0   1
        { 0, 15},     // Palette 1
        {15,  0},     // Palette 2
        { 0,  2},     // Palette 3
        { 2,  0}      // Palette 4
    };

    static const uint32_t agat_apple_colors_ind[2][2] = {
        // Hi  0           1
        {5, 4},   // Even
        {2, 1}     // Odd
    };

    static const uint32_t agat_apple_colors_NTSC[2][2] = {
        // Hi   0           1
        {0xFFFF63C0, 0xFFEFA943},   // Even
        {0xFF16C265, 0xFF367CE2}    // Odd
    };

    static const uint32_t agat_apple_hires_colors[16] = {
        0xFF000000, 0xFFC14843, 0xFF447715, 0xFFEFA943,
        0xFF006065, 0xFF929292, 0xFF16C265, 0xFFBAEE8C,
        0xFF643193, 0xFFFF63C0, 0xFF929292, 0xFFFFBEB9,
        0xFF367CE2, 0xFFDBA7FF, 0xFF5ED7DC, 0xFFFFFFFF
    };

    static const uint32_t agat_apple_hires_bw[2] = {
        0xFF000000, 0xFFFFFFFF
    };

    static const uint32_t agat_apple_lores_colors[16] = {
        0xFF000000, 0xFF643193, 0xFFC14843, 0xFFFF63C0,
        0xFF447715, 0xFF929292, 0xFFEFA943, 0xFFFFBEB9,
        0xFF006065, 0xFF367CE2, 0xFF929292, 0xFFDBA7FF,
        0xFF16C265, 0xFF5ED7DC, 0xFFBAEE8C, 0xFFFFFFFF
    };

}
