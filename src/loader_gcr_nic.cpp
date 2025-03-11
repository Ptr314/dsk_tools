#include "loader_gcr_nic.h"

namespace dsk_tools {
    LoaderGCR_NIC::LoaderGCR_NIC(std::string file_name, std::string format_id, std::string type_id):
        LoaderGCR(file_name, format_id, type_id)
    {}

    int LoaderGCR_NIC::get_track_len(int track)
    {
        return 512*16;
    }

    int LoaderGCR_NIC::get_track_offset(int track)
    {
        return track * get_track_len(track);
    }
}
