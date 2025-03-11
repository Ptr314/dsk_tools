#include "loader_gcr_nib.h"

namespace dsk_tools {
    LoaderGCR_NIB::LoaderGCR_NIB(std::string file_name, std::string format_id, std::string type_id):
        LoaderGCR(file_name, format_id, type_id)
    {}

    int LoaderGCR_NIB::get_track_len(int track)
    {
        return 416*16;
    }

    int LoaderGCR_NIB::get_track_offset(int track)
    {
        return track * get_track_len(track);
    }
}
