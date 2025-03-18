#ifndef LOADER_GCR_H
#define LOADER_GCR_H

#include "loader.h"

namespace dsk_tools {

    class LoaderGCR:public Loader
    {
    public:
        LoaderGCR(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual ~LoaderGCR() = default;
        virtual int load(std::vector<uint8_t> &buffer) override;
        virtual std::string file_info() override;
    protected:
        virtual void prepare_tracks_list(BYTES & in) {};
        virtual int get_track_len(int track) = 0;
        virtual int get_track_offset(int track);
    };

}

#endif // LOADER_GCR_H
