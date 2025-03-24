#ifndef WRITER_HXC_MFM_H
#define WRITER_HXC_MFM_H

#include "writer_mfm.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    struct HXC_MFM_HEADER
    {
        uint8_t  headername[7];          // "HXCMFM\0"

        uint16_t number_of_track;
        uint8_t  number_of_side;         // Number of elements in the MFMTRACKIMG array : number_of_track * number_of_side

        uint16_t floppyRPM;              // Rotation per minute.
        uint16_t floppyBitRate;          // 250 = 250Kbits/s, 300 = 300Kbits/s...
        uint8_t  floppyiftype;

        uint32_t mfmtracklistoffset;    // Offset of the MFMTRACKIMG array from the beginning of the file in number of uint8_ts.
    };

    struct HXC_MFM_TRACK_INFO
    {
        uint16_t track_number;
        uint8_t  side_number;
        uint32_t mfmtracksize;          // MFM/FM Track size in bytes
        uint32_t mfmtrackoffset;        // Offset of the track data from the beginning of the file in number of bytes.
    };

    #pragma pack(pop)

    class WriterHxCMFM:public WriterMFM
    {

    protected:
        void write_hxc_mfm_header(BYTES & out)        ;
    public:
        WriterHxCMFM(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id);
        virtual std::string get_default_ext() override;
        virtual int write(BYTES & buffer) override;
        virtual int substitute_tracks(BYTES & buffer, std::vector<uint8_t> &tmplt, const int numtracks) override;
    };

}

#endif // WRITER_HXC_MFM_H
