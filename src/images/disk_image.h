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

    #define ISOIBM_MFM_ENCODING				0x00
    #define AMIGA_MFM_ENCODING				0x01
    #define ISOIBM_FM_ENCODING				0x02
    #define EMU_FM_ENCODING					0x03
    #define UNKNOWN_ENCODING				0xFF

    #define IBMPC_DD_FLOPPYMODE				0x00
    #define IBMPC_HD_FLOPPYMODE				0x01
    #define ATARIST_DD_FLOPPYMODE			0x02
    #define ATARIST_HD_FLOPPYMODE			0x03
    #define AMIGA_DD_FLOPPYMODE				0x04
    #define AMIGA_HD_FLOPPYMODE				0x05
    #define CPC_DD_FLOPPYMODE				0x06
    #define GENERIC_SHUGGART_DD_FLOPPYMODE	0x07
    #define IBMPC_ED_FLOPPYMODE				0x08
    #define MSX2_DD_FLOPPYMODE				0x09
    #define C64_DD_FLOPPYMODE				0x0A
    #define EMU_SHUGART_FLOPPYMODE			0x0B
    #define S950_DD_FLOPPYMODE				0x0C
    #define S950_HD_FLOPPYMODE				0x0D
    #define DISABLE_FLOPPYMODE				0xFE


    class diskImage {
        protected:
            std::string m_type_id;
            BYTES m_buffer;
            std::unique_ptr<Loader> m_loader;
            unsigned m_format_heads;
            unsigned m_format_tracks;
            unsigned m_format_sectors;
            unsigned m_format_sector_size;
            unsigned m_expected_size;
            unsigned m_format_bitrate;
            unsigned m_format_rpm;
            unsigned m_format_track_encoding;
            unsigned m_format_floppyinterfacemode;
            bool m_is_loaded;
            bool m_sides_interleaved;
            std::vector<unsigned> m_sector_translation;

        public:
            explicit diskImage(std::unique_ptr<Loader> loader);
            diskImage(std::unique_ptr<Loader> loader, unsigned heads, unsigned tracks, unsigned sectors, unsigned sector_size,
                      unsigned bitrate, unsigned rpm, unsigned track_encoding, unsigned floppyinterfacemode,
                      bool sides_interleaved = true, const std::vector<unsigned> &sector_translation = {});
            void set_sector_translation(const std::vector<unsigned> &table);
            virtual ~diskImage();
            static unsigned transform_index(unsigned x, unsigned mod);
            virtual unsigned physical_sector(unsigned logical) const;
            virtual Result check();                                            // Check physical image parameters
            virtual Result load();
            virtual uint8_t *get_sector_data(unsigned head, unsigned track, unsigned sector);      // Uses sector translation

            std::string file_name() {return m_loader->get_file_name();};
            bool get_loaded() const {return m_is_loaded;};
            unsigned get_heads() const {return m_format_heads;};
            unsigned get_tracks() const {return m_format_tracks;};
            unsigned get_sectors() const {return m_format_sectors;};
            unsigned get_sector_size() const {return m_format_sector_size;};
            unsigned get_size() const {return m_expected_size;};
            unsigned get_bitrate() const {return m_format_bitrate;};
            unsigned get_rpm() const {return m_format_rpm;};
            unsigned get_track_encoding() const {return m_format_track_encoding;};
            unsigned get_floppyinterfacemode() const {return m_format_floppyinterfacemode;};
            std::vector<unsigned> get_sector_translation() const {return m_sector_translation;};
            std::string get_type_id() {return m_type_id;};
            BYTES * get_buffer() {return &m_buffer;};
            bool has_bad_sectors() const;
            bool is_bad_sector(unsigned head, unsigned track, unsigned sector) const;
    };
}
