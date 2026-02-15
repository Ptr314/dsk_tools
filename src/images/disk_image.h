// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for all disk images
#pragma once


#include <memory>
#include <vector>

#include "definitions.h"
#include "loader.h"

namespace dsk_tools {

    class diskImage {
        protected:
            std::string m_type_id;
            BYTES m_buffer;
            std::unique_ptr<Loader> m_loader;
            DiskFormatParams m_format;
            bool m_is_loaded;

        public:
            explicit diskImage(std::unique_ptr<Loader> loader);
            diskImage(std::unique_ptr<Loader> loader, const DiskFormatParams &format);
            void set_sector_translation(const std::vector<unsigned> &table);
            virtual ~diskImage();
            static unsigned transform_index(unsigned x, unsigned mod);
            virtual unsigned physical_sector(unsigned logical) const;
            virtual Result check();                                            // Check physical image parameters
            virtual Result load();
            virtual uint8_t *get_sector_data(unsigned head, unsigned track, unsigned sector);      // Uses sector translation

            std::string file_name() {return m_loader->get_file_name();};
            bool get_loaded() const {return m_is_loaded;};
            const DiskFormatParams& get_format() const {return m_format;};
            unsigned get_heads() const {return m_format.heads;};
            unsigned get_tracks() const {return m_format.tracks;};
            unsigned get_sectors() const {return m_format.sectors;};
            unsigned get_sector_size() const {return m_format.sector_size;};
            unsigned get_size() const {return m_format.expected_size;};
            unsigned get_bitrate() const {return m_format.bitrate;};
            unsigned get_rpm() const {return m_format.rpm;};
            unsigned get_track_encoding() const {return m_format.track_encoding;};
            unsigned get_floppyinterfacemode() const {return m_format.floppyinterfacemode;};
            std::vector<unsigned> get_sector_translation() const {return m_format.sector_translation;};
            std::string get_type_id() {return m_type_id;};
            BYTES * get_buffer() {return &m_buffer;};
            bool has_bad_sectors() const;
            bool is_bad_sector(unsigned head, unsigned track, unsigned sector) const;
            void logical_to_physical(unsigned & head, unsigned & track, unsigned & sector) const;
    };
}
