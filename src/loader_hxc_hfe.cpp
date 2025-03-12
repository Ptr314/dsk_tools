#include <fstream>
#include <iostream>

#include "definitions.h"
#include "utils.h"
#include "loader_hxc_hfe.h"

namespace dsk_tools {
LoaderHXC_HFE::LoaderHXC_HFE(std::string file_name, std::string format_id, std::string type_id):
        Loader(file_name, format_id, type_id)
    {}

    int LoaderHXC_HFE::load(std::vector<uint8_t> &buffer)
    {
        uint8_t res = FDD_LOAD_OK;

        // std::ifstream file(file_name, std::ios::binary);

        // if (!file.good()) {
        //     return FDD_LOAD_ERROR;
        // }

        // file.seekg (0, file.end);
        // auto fsize = file.tellg();
        // file.seekg (0, file.beg);

        // buffer.resize(fsize);
        // file.read (reinterpret_cast<char*>(buffer.data()), fsize);

        // loaded = true;

        return FDD_LOAD_OK;
    }

    std::string LoaderHXC_HFE::file_info()
    {
        std::string result = "";

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}:\n";
            return result;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(fsize) + " {$BYTES}\n";

        BYTES in(fsize);
        bool errors = false;

        file.read (reinterpret_cast<char*>(in.data()), fsize);

        HXC_HFE_HEADER * hdr = reinterpret_cast<HXC_HFE_HEADER*>(in.data());

        std::string signature(reinterpret_cast<char*>(&hdr->HEADERSIGNATURE), sizeof(hdr->HEADERSIGNATURE));

        if (signature == "HXCPICFE") {

            result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(0)) + " {$HEADER}\n";

            result += "    {$SIGNATURE}: " + signature + "\n";
            result += "    {$FORMAT_REVISION}: " + std::to_string(hdr->formatrevision) + "\n";
            result += "    {$TRACKS}: " + std::to_string(hdr->number_of_track) + "\n";
            result += "    {$SIDES}: " + std::to_string(hdr->number_of_side) + "\n";
            result += "    {$TRACKLIST_OFFSET}: " + std::to_string(hdr->track_list_offset) + "*512 ($" + dsk_tools::int_to_hex(hdr->track_list_offset*512, false) + ")\n";
        } else {
            result += "\n{$NO_SIGNATURE}\n";
            return result;
        }

        return result;

    }

}

