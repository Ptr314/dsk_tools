// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for all disk images

#include <iostream>

#include "disk_image.h"
#include "utils.h"

namespace dsk_tools {

    diskImage::diskImage(std::unique_ptr<Loader> loader):
          m_loader(std::move(loader))
        , m_is_loaded(false)
    {}

    diskImage::diskImage(std::unique_ptr<Loader> loader, const DiskFormatParams &format):
          m_loader(std::move(loader))
        , m_format(format)
        , m_is_loaded(false)
    {}

    diskImage::~diskImage() = default;

    Result diskImage::load()
    {
        if (!m_format.sector_translation.empty() && m_format.sector_translation.size() != m_format.sectors)
            return Result::error(ErrorCode::LoadError, "Sector translation table has incorrect size");

        m_type_id = m_loader->get_type_id();
        Result result = m_loader->load(m_buffer, m_format);
        if (result) {
            unsigned buffer_size = m_buffer.size();
            if (m_format.expected_size == 0 || (buffer_size >= m_format.expected_size && buffer_size <= m_format.expected_size + 4)) {
                m_is_loaded = true;
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

    unsigned diskImage::physical_sector(const unsigned logical) const {
        if (!m_format.sector_translation.empty())
            return m_format.sector_translation[logical];
        return logical;
    }

    void diskImage::set_sector_translation(const std::vector<unsigned> &table) {
        m_format.sector_translation = table;
    }

    uint8_t * diskImage::get_sector_data(const unsigned head, const unsigned track, const unsigned sector)
    {
        unsigned track_index = track * m_format.heads + head;
        if (m_format.heads == 2 && !m_format.sides_interleaved) track_index = transform_index(track_index, m_format.heads * m_format.tracks - 1);
        const unsigned sector_index = track_index * m_format.sectors + physical_sector(sector);
        const unsigned offset = sector_index * m_format.sector_size;

        // Bounds check: ensure offset + sector_size doesn't exceed buffer
        if (offset + m_format.sector_size > m_buffer.size()) {
            return nullptr;
        }
        return &m_buffer[offset];
    }

    bool diskImage::has_bad_sectors() const
    {
        return !m_loader->bad_sectors().empty();
    }

    bool diskImage::is_bad_sector(const unsigned head, const unsigned track, const unsigned sector) const
    {
        if (m_loader->bad_sectors().empty()) return false;

        unsigned new_head = head;
        unsigned new_track = track;
        unsigned new_sector = sector;

        logical_to_physical(new_head, new_track, new_sector);
        return m_loader->bad_sectors().count(bad_sector_key(new_head, new_track, new_sector)) > 0;

        // if (m_format.heads == 1 || m_format.sides_interleaved)
        //     return m_loader->bad_sectors().count(bad_sector_key(head, track, sector+m_format.sector_base)) > 0;
        //
        // // Two sides with sequential tracks
        // unsigned track_index = track * m_format.heads + head;
        // track_index = transform_index(track_index, m_format.heads * m_format.tracks - 1);
        // const unsigned new_head = track_index & 1;
        // const unsigned new_track = track_index >> 1;
        // return m_loader->bad_sectors().count(
        //     bad_sector_key(
        //             new_head,
        //             new_track,
        //             physical_sector(sector+m_format.sector_base)
        //     )
        // ) > 0;
    }
    void diskImage::logical_to_physical(unsigned & head, unsigned & track, unsigned & sector) const {
        if (m_format.heads == 2 && !m_format.sides_interleaved) {
            unsigned track_index = track * m_format.heads + head;
            track_index = transform_index(track_index, m_format.heads * m_format.tracks - 1);
            head = track_index & 1;
            track = track_index >> 1;
        }
        sector = physical_sector(sector+m_format.sector_base);
    }


    Result diskImage::check()
    {
        return Result::ok();
    }

}
