#ifndef WRITER_HXC_HFE_H
#define WRITER_HXC_HFE_H

#include "writer_mfm.h"

namespace dsk_tools {

    #define HFE_BLOCK_SIZE  512
    #define HFE_TRACK_LEN   26112

    class WriterHxCHFE:public WriterMFM
    {

    protected:
        void write_hxc_hfe_header(BYTES & out);
        void write_hxc_hfe_tracks_lut(BYTES & out);
    public:
        WriterHxCHFE(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id, const std::string & interleaving_id);
        virtual std::string get_default_ext() override;
        virtual int write(BYTES & buffer) override;
        virtual int substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}

#endif // WRITER_HXC_HFE_H
