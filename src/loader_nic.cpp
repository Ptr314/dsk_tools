#include <stdexcept>

#include "loader_nic.h"

namespace dsk_tools {
    LoaderNIC::LoaderNIC(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        LoaderMFM(file_name, format_id, type_id)
    {
        if (type_id == "TYPE_AGAT_140") {
            m_tracks_count = 35;
            m_sectors_count = 16;
            m_track_len = 512*16;
        } else
            throw std::runtime_error("LoaderNIC: Incorrect type id");
    }

}
