#ifndef LOADER_GCR_MFM_H
#define LOADER_GCR_MFM_H

#include "loader_gcr.h"
#include "writer_hxc_mfm.h"

namespace dsk_tools {

    class LoaderGCR_MFM:public LoaderGCR
    {
        private:
            HXC_MFM_HEADER * hdr;
            HXC_MFM_TRACK_INFO * ti[200];
        public:
            LoaderGCR_MFM(std::string file_name, std::string format_id, std::string type_id);
            virtual ~LoaderGCR_MFM() = default;
        protected:
            virtual void prepare_tracks_list(BYTES &in) override;
            virtual int get_track_len(int track) override;
            virtual int get_track_offset(int track) override;
    };

}


#endif // LOADER_GCR_MFM_H
