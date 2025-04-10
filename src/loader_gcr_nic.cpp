#include "loader_gcr_nic.h"

namespace dsk_tools {
    LoaderGCR_NIC::LoaderGCR_NIC(const std::string & file_name, const std::string & format_id, const std::string & type_id):
        LoaderGCR(file_name, format_id, type_id)
    {}

    int LoaderGCR_NIC::get_track_len(int track)
    {
        return 512*16;
    }
}
