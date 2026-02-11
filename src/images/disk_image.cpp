// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for all disk images

#include "disk_image.h"

namespace dsk_tools {

    diskImage::diskImage(std::unique_ptr<Loader> loader):
          loader(std::move(loader))
        , is_loaded(false)
        , interleaved(true)
    {}

    diskImage::~diskImage() = default;

    Result diskImage::load()
    {
        type_id = loader->get_type_id();
        Result result = loader->load(buffer);
        if (result) {
            int buffer_size = buffer.size();
            if (expected_size == 0 || (buffer_size >= expected_size && buffer_size <= expected_size + 4)) {
                is_loaded = true;
                return Result::ok();
            } else {
                return Result::error(ErrorCode::LoadSizeMismatch, "Buffer size mismatch");
            }
        }
        return result;
    }

    unsigned diskImage::transform_index(const unsigned x, const unsigned mod){
        return (2 * x) % mod + (x / mod) * mod;
    }

    uint8_t * diskImage::get_sector_data(const unsigned head, const unsigned track, const unsigned sector)
    {
        unsigned track_index = track * format_heads + head;
        if (!interleaved) track_index = transform_index(track_index, format_heads * format_tracks - 1);
        const unsigned sector_index = track_index * format_sectors + sector;
        const unsigned offset =  sector_index * format_sector_size;

        // Bounds check: ensure offset + sector_size doesn't exceed buffer
        if (offset + format_sector_size > static_cast<long>(buffer.size())) {
            return nullptr;
        }
        return &buffer[offset];
    }

}
