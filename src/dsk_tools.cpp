#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "dsk_tools/dsk_tools.h"

namespace dsk_tools {

    static const unsigned char FlipBit1[4] = { 0, 2,  1,  3  };
    static const unsigned char FlipBit2[4] = { 0, 8,  4,  12 };
    static const unsigned char FlipBit3[4] = { 0, 32, 16, 48 };

    const uint8_t m_write_translate_table[64] =
        {
            0x96,0x97,0x9A,0x9B,0x9D,0x9E,0x9F,0xA6,
            0xA7,0xAB,0xAC,0xAD,0xAE,0xAF,0xB2,0xB3,
            0xB4,0xB5,0xB6,0xB7,0xB9,0xBA,0xBB,0xBC,
            0xBD,0xBE,0xBF,0xCB,0xCD,0xCE,0xCF,0xD3,
            0xD6,0xD7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,
            0xDF,0xE5,0xE6,0xE7,0xE9,0xEA,0xEB,0xEC,
            0xED,0xEE,0xEF,0xF2,0xF3,0xF4,0xF5,0xF6,
            0xF7,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };

    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id)
    {
        std::cout << file_name << std::endl;
        dsk_tools::LoaderRAW * loader;
        if (format_id == "FILE_RAW_MSB") {
            loader = new dsk_tools::LoaderRAW(file_name, format_id, type_id, true);
        } else {
            return nullptr;
        }

        if (type_id == "TYPE_AGAT_140") {
            dsk_tools::imageAgat140 * disk_image = new dsk_tools::imageAgat140(loader);
            return disk_image;
        } else
        if (type_id == "TYPE_AGAT_840") {
            dsk_tools::imageAgat840 * disk_image = new dsk_tools::imageAgat840(loader);
            return disk_image;
        }

        return nullptr;
    }

    dsk_tools::fileSystem * prepare_filesystem(diskImage *image, std::string filesystem_id)
    {
        dsk_tools::fileSystem * fs;
        if (filesystem_id == "FILESYSTEM_DOS33") {
            fs = new dsk_tools::fsDOS33(image);
        } else
        if (filesystem_id == "FILESYSTEM_SPRITE_OS") {
            fs = new dsk_tools::fsSpriteOS(image);
        } else {
            return nullptr;
        }

        return fs;
    }

    BYTES code44(const BYTES & buffer)
    {
        BYTES result;
        for (int i=0; i<buffer.size(); i++) {
            result.push_back((buffer[i] >> 1) | 0xaa);
            result.push_back( buffer[i]       | 0xaa);
        }

        return result;
    }

    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out)
    {

        // First 86 bytes are combined lower 2 bits of input data
        for (int i = 0; i < 86; i++) {
            data_out[i] = FlipBit1[data_in[i]&3] | FlipBit2[data_in[i+86]&3] | FlipBit3[data_in[(i+172) & 0xFF]&3];
                // ^^ 2 extra bytes are wrapped to the beginning
        }

        // Next 256 bytes are upper 6 bits
        for (int i = 0; i < 256; i++) {
            data_out[i+86] = data_in[i] >> 2;
        }

        // Then, encode 6 bits to 8 bits using a table and calculate a crc
        uint8_t crc = 0;
        for (int i = 0; i < 342; i++) {
            uint8_t v = data_out[i];
            data_out[i] = m_write_translate_table[v ^ crc];
            crc = v;
        }

        // And finally add a crc byte
        data_out[342] = m_write_translate_table[crc];
    }

    int detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id)
    {
        namespace fs = std::filesystem;

        std::string ext = fs::path(file_name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // format_if
        if (ext == ".dsk") {
            format_id = "FILE_RAW_MSB";
        } else
            return FDD_DETECT_ERROR;

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        // type_id
        if (format_id == "FILE_RAW_MSB" && fsize == 143360) {
            type_id = "TYPE_AGAT_140";
        } else
        if (format_id == "FILE_RAW_MSB" && (fsize == 860160 || fsize == 860164)) {
            type_id = "TYPE_AGAT_840";
        } else
            return FDD_DETECT_ERROR;

        // filesystem_id
        if (format_id == "FILE_RAW_MSB" && (type_id == "TYPE_AGAT_140" || type_id == "TYPE_AGAT_840")) {
            BYTES buffer(32);
            file.read (reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (buffer[0] == 0x01) {
                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return FDD_DETECT_ERROR;
        }

        return FDD_DETECT_OK;
    }


} // namespace
