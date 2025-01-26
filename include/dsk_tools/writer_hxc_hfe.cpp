#include "dsk_tools/writer_hxc_hfe.h"
#include <cstring>
#include <iostream>
#include <fstream>

namespace dsk_tools {

    WriterHxCHFE::WriterHxCHFE(const std::string & format_id, diskImage * image_to_save):
        WriterMFM(format_id, image_to_save)
    {}

    std::string WriterHxCHFE::get_default_ext()
    {
        return "hfe";
    }

    void WriterHxCHFE::write_hxc_hfe_header(std::vector<uint8_t> & out)
    {
        picfileformatheader header;
        memset(&header, 0xFF, sizeof(header));

        std::strncpy(reinterpret_cast<char *>(&header.HEADERSIGNATURE[0]), "HXCPICFE", 8);
        header.formatrevision = 0;
        header.number_of_track = image->get_tracks();
        header.number_of_side = image->get_heads();
        header.track_encoding = image->get_track_encoding();
        header.bitRate = image->get_bitrate();
        header.floppyRPM = image->get_rpm();
        header.floppyinterfacemode = image->get_floppyinterfacemode();
        header.write_protected = 0xFF;
        header.track_list_offset = 512;
        header.write_allowed = 0xFF;

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

    int WriterHxCHFE::write(const std::string & file_name)
    {
        std::cout << "Converting started" << std::endl;
        std::cout << format_id << std::endl;
        std::cout << file_name << std::endl;

        std::vector<uint8_t> buffer;

        buffer.reserve(buffer.size() + 512);

        write_hxc_hfe_header(buffer);

        std::ofstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_WRITE_ERROR;
        }

        file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

        file.close();

        return 0;
    }

}
