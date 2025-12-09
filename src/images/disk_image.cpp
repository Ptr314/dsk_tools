// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for all disk images

#include "disk_image.h"

namespace dsk_tools {

    diskImage::diskImage(std::unique_ptr<Loader> loader):
          loader(std::move(loader))
        , is_loaded(false)
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

    uint8_t * diskImage::get_sector_data(int head, int track, int sector)
    {
        long offset = ((track * format_heads  + head) * format_sectors + sector) * format_sector_size;
        // Bounds check: ensure offset + sector_size doesn't exceed buffer
        if (offset < 0 || offset + format_sector_size > static_cast<long>(buffer.size())) {
            return nullptr;
        }
        return reinterpret_cast<uint8_t *>(&buffer[offset]);
    }

}
