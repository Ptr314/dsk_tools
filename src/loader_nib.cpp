// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .NIB files

#include <stdexcept>

#include "loader_nib.h"

namespace dsk_tools {
    LoaderNIB::LoaderNIB(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        LoaderMFM(file_name, format_id, type_id)
    {
        if (type_id == "TYPE_AGAT_140") {
            m_tracks_count = 35;
            m_sectors_count = 16;
            m_track_len = 416*m_sectors_count;
        } else
        if (type_id == "TYPE_AGAT_840") {
            m_tracks_count =  160;
            m_sectors_count = 21;
            m_track_len = 5922;         // TODO: Ensure for all
        } else
            throw std::runtime_error("LoaderNIB: Incorrect type id");
    }

}
