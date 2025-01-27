#include "dsk_tools/writer_hxc_mfm.h"
#include <cstring>
#include <iostream>
#include <fstream>

namespace dsk_tools {

WriterHxCMFM::WriterHxCMFM(const std::string & format_id, diskImage * image_to_save):
    WriterMFM(format_id, image_to_save)
{}

std::string WriterHxCMFM::get_default_ext()
{
    return "mfm";
}

void WriterHxCMFM::write_hxc_mfm_header(BYTES & out)
{
    HXC_MFM_HEADER header;

    std::strncpy(reinterpret_cast<char *>(&header.headername), "HXCMFM\0", 7);

    header.number_of_track = image->get_tracks();
    header.number_of_side = image->get_heads();
    header.floppyRPM = image->get_rpm();
    header.floppyBitRate = image->get_bitrate();
    header.floppyiftype = 0;
    header.mfmtracklistoffset = sizeof(header);

    uint8_t * ptr = reinterpret_cast<uint8_t*>(&header);
    out.insert(out.end(), ptr, ptr + sizeof(header));
}

int WriterHxCMFM::write(const std::string & file_name)
{
    // std::cout << "Converting started" << std::endl;
    // std::cout << format_id << std::endl;
    // std::cout << file_name << std::endl;

    BYTES buffer;

    buffer.reserve(buffer.size() + sizeof(HXC_MFM_HEADER));

    write_hxc_mfm_header(buffer);

    HXC_MFM_TRACK_INFO  hxc_mfm_track_info;

    int track_offset_mult = (image->get_heads()==2)?2:1;

    std::string type_id = image->get_type_id();

    int track_size;

    if (type_id == "TYPE_AGAT_140") {
        track_size = 6400;
    } else
        return FDD_WRITE_ERROR;

    buffer.reserve(0x800);

    for (uint8_t track = 0; track < image->get_tracks(); track++){
        for (uint8_t head = 0; head < image->get_heads(); head++){
            hxc_mfm_track_info.track_number = track;
            hxc_mfm_track_info.side_number = head;
            hxc_mfm_track_info.mfmtracksize = track_size;
            hxc_mfm_track_info.mfmtrackoffset = 0x800 + (track*track_offset_mult + head)*track_size;
            uint8_t * ptr = reinterpret_cast<uint8_t*>(&hxc_mfm_track_info);
            buffer.insert(buffer.end(), ptr, ptr+sizeof(HXC_MFM_TRACK_INFO));
        }
    }

    buffer.insert(buffer.end(), 0x800 - sizeof(HXC_MFM_HEADER) - sizeof(HXC_MFM_TRACK_INFO)*image->get_tracks()*image->get_heads(), 0x00);

    buffer.reserve(0x800 + image->get_tracks() * track_size);

    for (uint8_t track = 0; track < image->get_tracks(); track++){
        write_gcr62_track(buffer, track);
    }

    std::ofstream file(file_name, std::ios::binary);

    if (!file.good()) {
        return FDD_WRITE_ERROR;
    }

    file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

    file.close();

    return 0;
}

}
