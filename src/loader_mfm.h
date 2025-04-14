#ifndef LOADER_MFM_H
#define LOADER_MFM_H

#include "loader.h"

namespace dsk_tools {

    class LoaderMFM:public Loader
    {
    public:
        LoaderMFM(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual std::string file_info() override;
        virtual int load(std::vector<uint8_t> &buffer) override;

    protected:
        int m_track_offsets[200];
        int m_track_lengths[200];
        int m_tracks_count;
        int m_sectors_count;
        int m_track_len;

        using TrackInfoFunc = std::string (LoaderMFM::*)(BYTES&, int);
        using LoadTrackFunc = void (LoaderMFM::*)(int, BYTES&, const BYTES&, int);
        TrackInfoFunc track_info_func = nullptr;
        LoadTrackFunc load_track_func = nullptr;

        virtual int get_tracks_count() {return m_tracks_count;};
        virtual int get_sectors_count() {return m_sectors_count;};
        virtual int get_track_offset(int track) {return m_track_offsets[track];};
        virtual int get_track_len(int track) {return m_track_lengths[track];};
        virtual std::string get_header_info(BYTES & in) {return "";};
        virtual void prepare_tracks_list(BYTES & in);
        std::string agat140_track_info(BYTES & in, int track_len);
        std::string agat840_track_info(BYTES & in, int track_len);
        void load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len);
        void load_agat840_track(int track, BYTES & buffer, const BYTES & in, int track_len);
    };

}

#endif // LOADER_MFM_H
