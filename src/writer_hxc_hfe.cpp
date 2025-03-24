#include <cstring>
#include <iostream>
#include <fstream>

#include "writer_hxc_hfe.h"


namespace dsk_tools {

WriterHxCHFE::WriterHxCHFE(const std::string & format_id, diskImage * image_to_save, const uint8_t volume_id):
        WriterMFM(format_id, image_to_save, volume_id)
    {}

    std::string WriterHxCHFE::get_default_ext()
    {
        return "hfe";
    }

    void WriterHxCHFE::write_hxc_hfe_header(BYTES & out)
    {
        HXC_HFE_HEADER header;
        memset(&header, 0xFF, sizeof(header));

        std::strncpy(reinterpret_cast<char *>(&header.HEADERSIGNATURE[0]), "HXCPICFE", 8);
        header.formatrevision = 0;
        header.number_of_track = image->get_tracks();
        header.number_of_side = image->get_heads();
        header.track_encoding = image->get_track_encoding();
        header.bitRate = image->get_bitrate();
        header.floppyRPM = 0; //image->get_rpm();
        header.floppyinterfacemode = image->get_floppyinterfacemode();
        header.write_protected = 0x00;
        header.track_list_offset = 512 / HFE_BLOCK_SIZE;
        header.write_allowed = 0x00;

        // v1.1 extended fields
        header.single_step = 0xFF;
        header.track0s0_altencoding = 0xFF;
        header.track0s0_encoding = 0xFF;
        header.track0s1_altencoding = 0xFF;
        header.track0s1_encoding = 0xFF;

        uint8_t * ptr = reinterpret_cast<uint8_t*>(&header);
        out.insert(out.end(), ptr, ptr + sizeof(header));
        out.insert(out.end(), 512 - sizeof(header), 0xFF);
    }

    void WriterHxCHFE::write_hxc_hfe_tracks_lut(std::vector<uint8_t> & out)
    {
        HXC_HFE_TRACK track;
        uint8_t * ptr = reinterpret_cast<uint8_t*>(&track);
        int offset = 2;
        track.track_len = 12928 * 2; //TODO: calculate this value
        for (int i=0; i < image->get_tracks(); i++) {
            track.offset = offset;
            offset += HFE_TRACK_LEN / HFE_BLOCK_SIZE;

            out.insert(out.end(), ptr, ptr + sizeof(track));
        };

        out.insert(out.end(), HFE_BLOCK_SIZE - sizeof(track) * image->get_tracks(), 0xFF);
    }

    int WriterHxCHFE::write(BYTES &buffer)
    {
        buffer.clear();
        buffer.reserve(buffer.size() + 512);

        write_hxc_hfe_header(buffer);
        write_hxc_hfe_tracks_lut(buffer);

        BYTES track_buffer[image->get_heads()];

        for (uint8_t track = 0; track < image->get_tracks(); track++)
        {
            for (uint8_t head = 0; head < image->get_heads(); head++)
            {
                track_buffer[head].clear();
                write_agat840_track(track_buffer[head], head, track);
            }
            int blocks = 13056*2 / HFE_BLOCK_SIZE; // TODO: calculate
            for (int block=0; block < blocks; block++)
            {
                for (uint8_t head=0; head < 2; head ++)
                {
                    uint8_t * ptr = track_buffer[head].data() + block*256;
                    buffer.insert(buffer.end(), ptr, ptr+256);
                }
            }
        }
        return 0;
    }

    int WriterHxCHFE::substitute_tracks(BYTES & buffer, std::vector<uint8_t> &tmplt, const int numtracks)
    {
        return -1;
    }

}
