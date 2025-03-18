#include <iostream>
#include <fstream>

#include "dsk_tools/dsk_tools.h"

namespace dsk_tools {

    static const unsigned char FlipBit1[4] = { 0, 2,  1,  3  };
    static const unsigned char FlipBit2[4] = { 0, 8,  4,  12 };
    static const unsigned char FlipBit3[4] = { 0, 32, 16, 48 };

    static const uint8_t m_write_translate_table[64] =
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

    static const uint8_t m_read_translate_table[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x03,0x00,0x04,0x05,0x06,
        0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x08,0x00,0x00,0x00,0x09,0x0a,0x0b,0x0c,0x0d,
        0x00,0x00,0x0e,0x0f,0x10,0x11,0x12,0x13,0x00,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1b,0x00,0x1c,0x1d,0x1e,
        0x00,0x00,0x00,0x1f,0x00,0x00,0x20,0x21,0x00,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
        0x00,0x00,0x00,0x00,0x00,0x29,0x2a,0x2b,0x00,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,
        0x00,0x00,0x33,0x34,0x35,0x36,0x37,0x38,0x00,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
    };


    dsk_tools::diskImage *  prepare_image(std::string file_name, std::string format_id, std::string type_id)
    {
        std::cout << file_name << std::endl;
        dsk_tools::Loader * loader;
        if (format_id == "FILE_RAW_MSB") {
            loader = new dsk_tools::LoaderRAW(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_AIM") {
            loader = new dsk_tools::LoaderAIM(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_MFM_NIC") {
            loader = new dsk_tools::LoaderGCR_NIC(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_MFM_NIB") {
            loader = new dsk_tools::LoaderGCR_NIB(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_HXC_MFM") {
            loader = new dsk_tools::LoaderGCR_MFM(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_HXC_HFE") {
            loader = new dsk_tools::LoaderHXC_HFE(file_name, format_id, type_id);
        } else
            return nullptr;

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

    BYTES decode44(const BYTES & buffer)
    {
        BYTES result;
        for (int i=0; i<buffer.size()/2; i++) {
            uint8_t b1 = buffer[i*2]   & 0x55;
            uint8_t b2 = buffer[i*2+1] & 0x55;
            result.push_back((b1 << 1) | b2);
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

    bool decode_gcr62(const uint8_t data_in[], uint8_t * data_out)
    {
        uint8_t crc = 0;

        for (int i=0; i<86; i++) {
            uint8_t x = (crc^m_read_translate_table[data_in[i]]) & 0x3f;
            data_out[i+172] = FlipBit1[(x>>4) & 3];
            data_out[i+86] =  FlipBit1[(x>>2) & 3];
            data_out[i] =     FlipBit1[ x     & 3];
            crc = x;
        }
        for (int i=0; i<256; i++) {
            uint8_t x = (crc^m_read_translate_table[data_in[i+86]]) & 0x3f;
            data_out[i] |=  x << 2 ;
            crc = x;
        }

        uint8_t r_crc = m_read_translate_table[data_in[342]];

        return crc == r_crc;
    }

    uint16_t encode_agat_MFM_byte(uint8_t data, uint8_t & last_byte)
    {
        uint16_t mfm_encoded;
        mfm_encoded = agat_MFM_tab[((last_byte & 1) << 8) + data];

        last_byte = data;

        return (mfm_encoded << 8) + (mfm_encoded >> 8);
    }


    uint8_t decode_agat_MFM_byte(uint8_t data)
    {
        return agat_MFM_decode_tab[data >> 1];
    }

    void encode_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t & last_byte)
    {
        uint16_t mfm_word;
        for (int i=0; i<count; i++) {
            mfm_word = encode_agat_MFM_byte(data, last_byte);
            out.push_back(mfm_word & 0xFF);
            out.push_back((mfm_word >> 8) & 0xFF);
        }
    }

    uint8_t encode_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t & last_byte)
    {
        uint16_t mfm_word;
        uint16_t crc = 0;
        for (int i=0; i<count; i++) {
            mfm_word = encode_agat_MFM_byte(data[i], last_byte);
            out.push_back(mfm_word & 0xFF);
            out.push_back((mfm_word >> 8) & 0xFF);
            if (crc > 0xFF) crc = (crc + 1) & 0xFF;
            crc += data[i];
        }
        return crc & 0xFF;
    }

    void decode_agat_mfm_data(BYTES & out, const BYTES & in) {
        out.clear();

        for (int i=0; i<in.size()/2; i++)
        {
            uint8_t b1 = in[i*2];
            uint8_t b2 = in[i*2+1];
            uint8_t b =  (agat_MFM_decode_tab[b1>>1] << 4) | agat_MFM_decode_tab[b2>>1];
            out.push_back(b);
        }
    }


    int detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id)
    {
        std::string ext = get_file_ext(file_name);

        // format_if
        if (ext == ".dsk") {
            format_id = "FILE_RAW_MSB";

            std::ifstream file(file_name, std::ios::binary);

            if (!file.good()) {
                return FDD_LOAD_ERROR;
            }

            file.seekg (0, file.end);
            auto fsize = file.tellg();
            file.seekg (0, file.beg);

            // type_id
            if (fsize == 143360) {
                type_id = "TYPE_AGAT_140";
            } else
            if (fsize == 860160 || fsize == 860164) {
                type_id = "TYPE_AGAT_840";
            } else
                return FDD_DETECT_ERROR;

            // filesystem_id
            if (type_id == "TYPE_AGAT_140" || type_id == "TYPE_AGAT_840") {
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

        } else
        if (ext == ".aim") {
            format_id = "FILE_AIM";
            type_id = "TYPE_AGAT_840";
            dsk_tools::LoaderAIM loader(file_name, format_id, type_id);
            BYTES buffer;
            loader.load(buffer);
            if (buffer[0] == 0x01) {
                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return FDD_DETECT_ERROR;
        } else
        if (ext == ".nic" || ext == ".nib" || ext == ".mfm") {
            type_id = "TYPE_AGAT_140";

            BYTES buffer;
            if (ext == ".nic") {
                format_id = "FILE_MFM_NIC";
                dsk_tools::LoaderGCR_NIC loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
            if (ext == ".nib") {
                format_id = "FILE_MFM_NIB";
                dsk_tools::LoaderGCR_NIB loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
            if (ext == ".mfm") {
                format_id = "FILE_HXC_MFM";
                dsk_tools::LoaderGCR_MFM loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
                return FDD_DETECT_ERROR;

            if (buffer[0] == 0x01) {
                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return FDD_DETECT_ERROR;
        } else
        if (ext == ".hfe") {
            format_id = "FILE_HXC_HFE";

            std::ifstream file(file_name, std::ios::binary);

            if (!file.good()) {
                return FDD_LOAD_ERROR;
            }

            BYTES hdr_buffer(sizeof(HXC_HFE_HEADER));
            file.read (reinterpret_cast<char*>(hdr_buffer.data()), hdr_buffer.size());
            HXC_HFE_HEADER * hdr = reinterpret_cast<HXC_HFE_HEADER*>(hdr_buffer.data());

            if (hdr->number_of_side == 2 && hdr->number_of_track == 80) {
                type_id = "TYPE_AGAT_840";
                BYTES buffer(sizeof(HXC_HFE_HEADER));
                dsk_tools::LoaderHXC_HFE loader(file_name, format_id, type_id);
                loader.load(buffer);
                if (buffer[0] == 0x01) {
                    if (buffer[2] == 0x58) {
                        filesystem_id = "FILESYSTEM_SPRITE_OS";
                    } else {
                        filesystem_id = "FILESYSTEM_DOS33";
                    }
                } else
                    return FDD_DETECT_ERROR;
            }
        } else
            return FDD_DETECT_ERROR;

        return FDD_DETECT_OK;
    }


} // namespace
