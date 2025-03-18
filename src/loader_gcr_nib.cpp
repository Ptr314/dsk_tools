#include "loader_gcr_nib.h"

namespace dsk_tools {
LoaderGCR_NIB::LoaderGCR_NIB(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        LoaderGCR(file_name, format_id, type_id)
    {}

    int LoaderGCR_NIB::get_track_len(int track)
    {
        return 416*16;
    }
}
