#ifndef WRITER_HXC_HFE_H
#define WRITER_HXC_HFE_H

#include "dsk_tools/writer_mfm.h"

namespace dsk_tools {

#pragma pack(push, 1)

    struct picfileformatheader
    {
        uint8_t  HEADERSIGNATURE[8];
        uint8_t  formatrevision;
        uint8_t  number_of_track;
        uint8_t  number_of_side;
        uint8_t  track_encoding;
        uint16_t bitRate;
        uint16_t floppyRPM;
        uint8_t  floppyinterfacemode;
        uint8_t  write_protected;
        uint16_t track_list_offset;
        uint8_t  write_allowed;
        uint8_t  single_step;
        uint8_t  track0s0_altencoding;
        uint8_t  track0s0_encoding;
        uint8_t  track0s1_altencoding;
        uint8_t  track0s1_encoding;
    };

#pragma pack(pop)

    class WriterHxCHFE:public WriterMFM
    {

    protected:
        void write_hxc_hfe_header(std::vector<uint8_t> & out)        ;
    public:
        WriterHxCHFE(const std::string & format_id, diskImage *image_to_save);
        virtual std::string get_default_ext() override;
        virtual int write(const std::string & file_name) override;
    };

}

#endif // WRITER_HXC_HFE_H
