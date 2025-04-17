// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .MFM files

#include <cstring>
#include <stdexcept>

#include "loader_hxc_mfm.h"
#include "utils.h"
#include "writer_hxc_mfm.h"

namespace dsk_tools {
    LoaderHXC_MFM::LoaderHXC_MFM(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        LoaderMFM(file_name, format_id, type_id)
    {
        if (type_id == "TYPE_AGAT_140") {
            m_tracks_count = 35;
            m_sectors_count = 16;
            m_track_len = 0;
        } else
            throw std::runtime_error("LoaderHXC_MFM: Incorrect type id");
    }

    void LoaderHXC_MFM::prepare_tracks_list(BYTES & in)
    {
        HXC_MFM_HEADER * hdr = reinterpret_cast<HXC_MFM_HEADER *>(in.data());

        if (hdr->number_of_track != m_tracks_count)
            throw std::runtime_error("LoaderHXC_MFM: Incorrect tracks number");

        for (int track=0; track<m_tracks_count; track++) {
            HXC_MFM_TRACK_INFO * ti = reinterpret_cast<HXC_MFM_TRACK_INFO *>(in.data() + hdr->mfmtracklistoffset + track * sizeof(HXC_MFM_TRACK_INFO));
            m_track_offsets[track] = ti->mfmtrackoffset;
            m_track_lengths[track] = ti->mfmtracksize;
        }
    }

    std::string LoaderHXC_MFM::get_header_info(BYTES & in)
    {
        std::string result;
        HXC_MFM_HEADER * hdr = reinterpret_cast<HXC_MFM_HEADER *>(in.data());

        result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(0)) + " {$HEADER}\n";

        result += "    {$TRACKS}: " + std::to_string(hdr->number_of_track) + "\n";
        result += "    {$SIDES}: " + std::to_string(hdr->number_of_side) + "\n";
        result += "\n";

        result += "$" + dsk_tools::int_to_hex(hdr->mfmtracklistoffset) + " {$TRACKLIST_OFFSET}\n";

        for (int track=0; track<hdr->number_of_track; track++) {
            HXC_MFM_TRACK_INFO * ti = reinterpret_cast<HXC_MFM_TRACK_INFO *>(in.data() + hdr->mfmtracklistoffset + track * sizeof(HXC_MFM_TRACK_INFO));
            result += "    {$SIDE_SHORT}: " + std::to_string(ti->side_number)
                      + ", {$TRACK_SHORT}=" + std::to_string(ti->track_number)
                      + ", {$TRACK_OFFSET}: $" + dsk_tools::int_to_hex(ti->mfmtrackoffset, false)
                      + ", {$TRACK_SIZE}: $" + dsk_tools::int_to_hex(ti->mfmtracksize, false)
                      + "\n";
        }
        result += "\n";

        return result;

    }
}
