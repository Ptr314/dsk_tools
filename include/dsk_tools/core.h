// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: The part of the library that needs no loaders, images, file systems
// or viewers: definitions, string and host file helpers, track encodings.
// Built alone as the dsk_tools_core target; dsk_tools.h includes it

#pragma once

#include <string>

#include "definitions.h"
#include "utils.h"
#include "bit_enums.h"
#include "host_helpers.h"

namespace dsk_tools {

    BYTES code44(const BYTES & buffer);
    BYTES decode44(const BYTES & buffer);
    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    bool decode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    uint16_t encode_agat_MFM_byte(uint8_t data, uint8_t &last_byte);
    uint8_t decode_agat_MFM_byte(uint8_t data);
    void encode_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t & last_byte);
    uint8_t encode_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t & last_byte);
    void decode_agat_mfm_data(BYTES &out, const BYTES & in);
    int detect_agat_sector_size(const BYTES & in);
    Result decode_agat_840_track(BYTES &out, const BYTES & in, const int sectors = 21, const int sector_size = 256);
    Result decode_agat_840_image(BYTES &out, const BYTES & in);

    Result load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len);
    Result decode_agat_140_image(BYTES &out, const BYTES & in, const int track_len);

} // namespace
