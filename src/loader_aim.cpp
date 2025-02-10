#include <fstream>
#include <iostream>

#include "loader_aim.h"

namespace dsk_tools {
    LoaderAIM::LoaderAIM(std::string file_name, std::string format_id, std::string type_id):
        Loader(file_name, format_id, type_id)
    {}

    LoaderAIM::~LoaderAIM()
    {}

    bool LoaderAIM::iterate_until(const std::vector<uint16_t> & in, int & p, const uint8_t v)
    {
        uint16_t b;
        //uint8_t c;
        uint8_t d;
        do {
            b = in.at(p++);
            //c = b >> 8;
            d = b & 0xFF;
            if (p >= in.size()) return false;
        } while (d != v);
        return true;
    }

    int LoaderAIM::load(BYTES & buffer)
    {
        uint8_t res = FDD_LOAD_OK;

        std::cout << "File name: " << file_name << std::endl;

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        std::cout << "File size: " << fsize << std::endl;

        int image_size = 160*21*256;
        buffer.resize(image_size);

        std::vector<uint16_t> in(fsize/2);

        file.read (reinterpret_cast<char*>(in.data()), fsize);

        int in_p = 0;
        int out_p = 0;

        for (int track=0; track<160; track++) {
            for (int sector=0; sector<21; sector++) {
                // Index
                if (!iterate_until(in, in_p, 0x95)) return FDD_LOAD_ERROR;
                if (!iterate_until(in, in_p, 0x6A)) return FDD_LOAD_ERROR;
                // VTS
                // uint8_t r_v = in.at(in_p++) & 0xFF;
                // uint8_t r_t = in.at(in_p++) & 0xFF;
                // uint8_t r_s = in.at(in_p++) & 0xFF;
                in_p += 3;
                // Data mark
                if (!iterate_until(in, in_p, 0x6A)) return FDD_LOAD_ERROR;
                if (!iterate_until(in, in_p, 0x95)) return FDD_LOAD_ERROR;
                // Data
                for (int i=0; i<256; i++) {
                    uint16_t b = in.at(in_p++);
                    uint8_t  d = b & 0xFF;
                    buffer[out_p++] = d;
                }
            }
        }

        loaded = true;

        return FDD_LOAD_OK;
    }
}
