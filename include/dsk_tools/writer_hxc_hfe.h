#ifndef WRITER_HXC_HFE_H
#define WRITER_HXC_HFE_H

#include "dsk_tools/writer_mfm.h"

namespace dsk_tools {

    #define HFE_BLOCK_SIZE  512
    #define HFE_TRACK_LEN   26112

    #pragma pack(push, 1)

    // https://hxc2001.com/floppy_drive_emulator/HFE-file-format.html
    struct HXC_HFE_HEADER
    {
        uint8_t  HEADERSIGNATURE[8];        // "HXCPICFE" for HFEv1 and HFEv2, "HXCHFEV3" for HFEv3
        uint8_t  formatrevision;            // 0 for the HFEv1, 1 for the HFEv2. Reset to 0 for HFEv3.
        uint8_t  number_of_track;           // Number of track(s) in the file
        uint8_t  number_of_side;            // Number of valid side(s)
        uint8_t  track_encoding;            // Track Encoding mode
        uint16_t bitRate;                   // Bitrate in Kbit/s
        uint16_t floppyRPM;                 // Rotation per minute
        uint8_t  floppyinterfacemode;       // Floppy interface mode
        uint8_t  write_protected;           // Reserved
        uint16_t track_list_offset;         // Offset of the track list LUT in block of 512 bytes
        uint8_t  write_allowed;             // 0x00 : Write protected, 0xFF: Unprotected
        // v1.1 addition
        uint8_t  single_step;               // 0xFF : Single Step - 0x00 Double Step mode
        uint8_t  track0s0_altencoding;      // 0x00 : Use an alternate track_encoding for track 0 Side 0
        uint8_t  track0s0_encoding;         // alternate track_encoding for track 0 Side 0
        uint8_t  track0s1_altencoding;      // 0x00 : Use an alternate track_encoding for track 0 Side 1
        uint8_t  track0s1_encoding;         // alternate track_encoding for track 0 Side 1
    };

    struct HXC_HFE_TRACK
    {
        uint16_t	offset;                 // Track data offset in block of 512 bytes (Ex: 2 = 0x400)
        uint16_t	track_len;              // Length of the track data in byte.
    };


    #pragma pack(pop)

    class WriterHxCHFE:public WriterMFM
    {

    protected:
        void write_hxc_hfe_header(BYTES & out);
        void write_hxc_hfe_tracks_lut(BYTES & out);
    public:
        WriterHxCHFE(const std::string & format_id, diskImage *image_to_save);
        virtual std::string get_default_ext() override;
        virtual int write(const std::string & file_name) override;
    };

}

#endif // WRITER_HXC_HFE_H
