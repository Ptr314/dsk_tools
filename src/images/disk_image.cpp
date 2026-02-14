// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for all disk images

#include "disk_image.h"
#include "utils.h"

namespace dsk_tools {

    diskImage::diskImage(std::unique_ptr<Loader> loader):
          m_loader(std::move(loader))
        , m_is_loaded(false)
        , m_sides_interleaved(true)
    {}

    diskImage::diskImage(std::unique_ptr<Loader> loader, unsigned heads, unsigned tracks, unsigned sectors, unsigned sector_size,
                         unsigned bitrate, unsigned rpm, unsigned track_encoding, unsigned floppyinterfacemode,
                         bool sides_interleaved, const std::vector<unsigned> &sector_translation):
          m_loader(std::move(loader))
        , m_format_heads(heads)
        , m_format_tracks(tracks)
        , m_format_sectors(sectors)
        , m_format_sector_size(sector_size)
        , m_expected_size(heads * tracks * sectors * sector_size)
        , m_format_bitrate(bitrate)
        , m_format_rpm(rpm)
        , m_format_track_encoding(track_encoding)
        , m_format_floppyinterfacemode(floppyinterfacemode)
        , m_is_loaded(false)
        , m_sides_interleaved(sides_interleaved)
        , m_sector_translation(sector_translation)
    {}

    diskImage::~diskImage() = default;

    Result diskImage::load()
    {
        if (m_sector_translation.size() > 0 && m_sector_translation.size() != m_format_sectors)
            return Result::error(ErrorCode::LoadError, "Sector translation table has incorrect size");

        m_type_id = m_loader->get_type_id();
        Result result = m_loader->load(m_buffer, m_format_heads, m_format_tracks, m_format_sectors, m_format_sector_size, m_expected_size);
        if (result) {
            unsigned buffer_size = m_buffer.size();
            if (m_expected_size == 0 || (buffer_size >= m_expected_size && buffer_size <= m_expected_size + 4)) {
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
        if (!m_sector_translation.empty())
            return m_sector_translation[logical];
        return logical;
    }

    void diskImage::set_sector_translation(const std::vector<unsigned> &table) {
        m_sector_translation = table;
    }

    uint8_t * diskImage::get_sector_data(const unsigned head, const unsigned track, const unsigned sector)
    {
        unsigned track_index = track * m_format_heads + head;
        if (m_format_heads == 2 && !m_sides_interleaved) track_index = transform_index(track_index, m_format_heads * m_format_tracks - 1);
        const unsigned sector_index = track_index * m_format_sectors + physical_sector(sector);
        const unsigned offset = sector_index * m_format_sector_size;

        // Bounds check: ensure offset + sector_size doesn't exceed buffer
        if (offset + m_format_sector_size > m_buffer.size()) {
            return nullptr;
        }
        return &m_buffer[offset];
    }

    bool diskImage::has_bad_sectors() const
    {
        return !m_loader->bad_sectors().empty();
    }

    bool diskImage::is_bad_sector(unsigned head, unsigned track, unsigned sector) const
    {
        return m_loader->bad_sectors().count(bad_sector_key(head, track, sector)) > 0;
    }

    Result diskImage::check()
    {
        return Result::ok();
    }

}
