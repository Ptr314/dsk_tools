#include "loader_gcr_mfm.h"

namespace dsk_tools {
    LoaderGCR_MFM::LoaderGCR_MFM(std::string file_name, std::string format_id, std::string type_id):
        LoaderGCR(file_name, format_id, type_id)
    {}

    void LoaderGCR_MFM::prepare_tracks_list(BYTES & in)
    {
        hdr = reinterpret_cast<HXC_MFM_HEADER *>(in.data());
        for (int track=0; track<hdr->number_of_track; track++) {
            ti[track] = reinterpret_cast<HXC_MFM_TRACK_INFO *>(in.data() + hdr->mfmtracklistoffset + track * sizeof(HXC_MFM_TRACK_INFO));
        }

    }

    int LoaderGCR_MFM::get_track_len(int track)
    {
        return ti[track]->mfmtracksize;
    }

    int LoaderGCR_MFM::get_track_offset(int track)
    {
        return ti[track]->mfmtrackoffset;
    }
}
