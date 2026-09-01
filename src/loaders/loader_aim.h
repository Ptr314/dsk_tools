// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for AIM (Agat 840/880 Kb psysical images)
#pragma once


#include "loader.h"

namespace dsk_tools {

    class LoaderAIM:public Loader
    {
    private:
        bool iterate_until(const std::vector<uint16_t> &in, int &p, const uint8_t v);
        static int detect_sector_size(const std::vector<uint16_t> & in);
    protected:
        bool msb_first;
        // Geometry of the last loaded image: 21x256 (840 Kb) or 11x512 (880 Kb)
        int m_sectors = 21;
        int m_sector_size = 256;
    public:
        LoaderAIM(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual ~LoaderAIM() = default;
        Result load(BYTES & buffer, const DiskFormatParams &format = DiskFormatParams()) override;
        std::string file_info() override;

        int get_sectors() const {return m_sectors;};
        int get_sector_size() const {return m_sector_size;};
    };

}
