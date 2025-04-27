// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Main source file

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


    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id)
    {
        // std::cout << file_name << std::endl;
        dsk_tools::Loader * loader;
        if (format_id == "FILE_RAW_MSB") {
            loader = new dsk_tools::LoaderRAW(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_AIM") {
            loader = new dsk_tools::LoaderAIM(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_MFM_NIC") {
            loader = new dsk_tools::LoaderNIC(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_MFM_NIB") {
            loader = new dsk_tools::LoaderNIB(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_HXC_MFM") {
            loader = new dsk_tools::LoaderHXC_MFM(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_HXC_HFE") {
            loader = new dsk_tools::LoaderHXC_HFE(file_name, format_id, type_id);
        } else
        if (format_id == "FILE_FIL") {
            loader = new dsk_tools::LoaderFIL(file_name, format_id, type_id);
        } else
            return nullptr;

        if (type_id == "TYPE_AGAT_140") {
            dsk_tools::imageAgat140 * disk_image = new dsk_tools::imageAgat140(loader);
            return disk_image;
        } else
        if (type_id == "TYPE_AGAT_840") {
            dsk_tools::imageAgat840 * disk_image = new dsk_tools::imageAgat840(loader);
            return disk_image;
        } else
        if (type_id == "TYPE_FIL") {
            dsk_tools::imageFIL * disk_image = new dsk_tools::imageFIL(loader);
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
        } else
        if (filesystem_id == "FILESYSTEM_CPM_DOS" || filesystem_id == "FILESYSTEM_CPM_PRODOS"|| filesystem_id == "FILESYSTEM_CPM_RAW") {
            fs = new dsk_tools::fsCPM(image, filesystem_id);
        } else
        if (filesystem_id == "FILESYSTEM_FIL") {
            fs = new dsk_tools::fsFIL(image);
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
            if (i+172 < 256)
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


    int detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id, bool format_only)
    {
        std::string ext = get_file_ext(file_name);

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        // format_if
        if (ext == ".dsk" || ext == ".do" || ext == ".po" || ext == ".cpm") {
            format_id = "FILE_RAW_MSB";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return FDD_DETECT_OK;
            }

            // type_id
            if (fsize == 143360 || fsize == 143360+128) {
                type_id = "TYPE_AGAT_140";
            } else
            if (fsize == 860160 || fsize == 860164) {
                type_id = "TYPE_AGAT_840";
            } else
                return FDD_DETECT_ERROR;

            // filesystem_id
            if (type_id == "TYPE_AGAT_140" || type_id == "TYPE_AGAT_840") {
                BYTES buffer(256);
                file.read (reinterpret_cast<char*>(buffer.data()), buffer.size());
                std::string ms = "MICROSOFT";
                std::string _ms(buffer.begin() + 0x74, buffer.begin() + 0x74 + ms.size());

                uint32_t vtoc_pos;
                if (type_id == "TYPE_AGAT_140") vtoc_pos=17*16*256;
                else
                if (type_id == "TYPE_AGAT_840") vtoc_pos=17*21*256;

                Agat_VTOC VTOC;
                file.seekg (vtoc_pos, file.beg);
                file.read (reinterpret_cast<char*>(&VTOC), sizeof(Agat_VTOC));

                if (buffer[0] == 0x01 && buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else
                if (type_id == "TYPE_AGAT_140" && _ms == ms) {
                    if (ext == ".po")
                        filesystem_id = "FILESYSTEM_CPM_PRODOS";
                    else
                        filesystem_id = "FILESYSTEM_CPM_DOS";
                } else
                if (type_id == "TYPE_AGAT_140" && ext == ".cpm") {
                    filesystem_id = "FILESYSTEM_CPM_RAW";
                } else
                    filesystem_id = "FILESYSTEM_DOS33";

                // } else
                //     return FDD_DETECT_ERROR;
            }

        } else
        if (ext == ".aim") {
            format_id = "FILE_AIM";

            type_id = "TYPE_AGAT_840";

            if (format_only) {
                filesystem_id = "";
                return FDD_DETECT_OK;
            }

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
            if (ext == ".nib") {
                if (fsize == 232960)
                    type_id = "TYPE_AGAT_140";
                else
                if (fsize == 947520)
                    type_id = "TYPE_AGAT_840";
                else
                    return FDD_DETECT_ERROR;
            } else
                type_id = "TYPE_AGAT_140";

            BYTES buffer;
            if (ext == ".nic") {
                format_id = "FILE_MFM_NIC";
                dsk_tools::LoaderNIC loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
            if (ext == ".nib") {
                format_id = "FILE_MFM_NIB";
                dsk_tools::LoaderNIB loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
            if (ext == ".mfm") {
                format_id = "FILE_HXC_MFM";
                dsk_tools::LoaderHXC_MFM loader(file_name, format_id, type_id);
                loader.load(buffer);
            } else
                return FDD_DETECT_ERROR;

            if (format_only) {
                filesystem_id = "";
                return FDD_DETECT_OK;
            }

            if (buffer[0] == 0x01) {
                std::string ms = "MICROSOFT";
                std::string _ms(buffer.begin() + 0x74, buffer.begin() + 0x74 + ms.size());

                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else
                if (type_id == "TYPE_AGAT_140" && _ms == ms) {
                    filesystem_id = "FILESYSTEM_CPM_DOS";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return FDD_DETECT_ERROR;
        } else
        if (ext == ".hfe") {
            format_id = "FILE_HXC_HFE";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return FDD_DETECT_OK;
            }

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
        if (ext == ".fil") {
            format_id = "FILE_FIL";
            type_id = "TYPE_FIL";
            filesystem_id = "FILESYSTEM_FIL";
        } else
            return FDD_DETECT_ERROR;

        return FDD_DETECT_OK;
    }

    std::string agat_vtoc_info(const Agat_VTOC & VTOC)
    {
        std::string result = "";
        if (VTOC.bytes_per_sector == 256 && VTOC.dos_release <= 3 ) {
            result += "{$VTOC_FOUND}\n";
        } else {
            result += "\n{$VTOC_NOT_FOUND}\n";
        }

        result += "    [$00]: $" + int_to_hex(VTOC._not_used_00) + " \n";
        result += "    {$VTOC_CATALOG_TRACK}: $" + int_to_hex(VTOC.catalog_track) + " (" + std::to_string(VTOC.catalog_track) + ") \n";
        result += "    {$VTOC_CATALOG_SECTOR}: $" + int_to_hex(VTOC.catalog_sector) + " (" + std::to_string(VTOC.catalog_sector) + ") \n";
        result += "    {$VTOC_DOS_RELEASE}: $" + int_to_hex(VTOC.dos_release) + " (" + std::to_string(VTOC.dos_release) + ") \n";
        result += "    [$04-05]: " + toHexList(&(VTOC._not_used_04[0]), 2, "$") + " \n";
        result += "    {$VTOC_VOLUME_ID}: $" + int_to_hex(VTOC.volume_id) + " (" + std::to_string(VTOC.volume_id) + ") \n";
        result += "    [$07]: $" + int_to_hex(VTOC._not_used_07) + " \n";
        result += "    {$VTOC_VOLUME_NAME}: «" + trim(agat_to_utf(VTOC.volume_name, 31)) + "» (" + toHexList(VTOC.volume_name, 31, "$") +")\n";
        result += "    {$VTOC_PAIRS_ON_SECTOR}: $" + int_to_hex(VTOC.pairs_on_sector) + " (" + std::to_string(VTOC.pairs_on_sector) + ") \n";
        result += "    [$28-2F]: " + toHexList(&(VTOC._not_used_28[0]), 8, "$") + " \n";
        result += "    {$VTOC_LAST_TRACK}: $" + int_to_hex(VTOC.last_track) + " (" + std::to_string(VTOC.last_track) + ") \n";
        result += "    {$VTOC_DIRECTION}: $" + int_to_hex(VTOC.direction) + " (" + std::to_string(VTOC.direction) + ") \n";
        result += "    [$32-33]: " + toHexList(&(VTOC._not_used_32[0]), 2, "$") + " \n";
        result += "    {$VTOC_TRACKS_TOTAL}: $" + int_to_hex(VTOC.tracks_total) + " (" + std::to_string(VTOC.tracks_total) + ") \n";
        result += "    {$VTOC_SECTORS_ON_TRACK}: $" + int_to_hex(VTOC.sectors_on_track) + " (" + std::to_string(VTOC.sectors_on_track) + ") \n";
        result += "    {$VTOC_BYTES_PER_SECTOR}: $" + int_to_hex(VTOC.bytes_per_sector) + " (" + std::to_string(VTOC.bytes_per_sector) + ") \n";

        return result;
    }

    std::string agat_sos_info(const SPRITE_OS_DPB_DISK & DPB)
    {
        std::string result = "";
        result += "{$DPB_INFO}\n";
        result += "    {$DPB_VOLUME_ID}: $" + int_to_hex(DPB.VOLUME) + " (" + std::to_string(DPB.VOLUME) + ") \n";
        result += "    {$DPB_TYPE}: $" + int_to_hex(DPB.TYPE) + " (" + std::to_string(DPB.TYPE) + ") \n";
        result += "    {$DPB_DSIDE}: $" + int_to_hex(DPB.DSIDE) + " (" + std::to_string(DPB.DSIDE) + ") \n";
        result += "    {$DPB_TSIZE}: $" + int_to_hex(DPB.TSIZE) + " (" + std::to_string(DPB.TSIZE) + ") \n";
        result += "    {$DPB_DSIZE}: $" + int_to_hex(DPB.DSIZE) + " (" + std::to_string(DPB.DSIZE) + ") \n";
        result += "    {$DPB_MAXBLOK}: $" + int_to_hex(DPB.MAXBLOK) + " (" + std::to_string(DPB.MAXBLOK) + ") \n";
        result += "    {$DPB_VTOCADR}: $" + int_to_hex(DPB.VTOCADR) + " (" + std::to_string(DPB.VTOCADR) + ") \n";
        return result;
    }

    std::pair<std::string, std::string> suggest_file_type(const BYTES & data)
    {
        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            const AGAT_EXIF_SECTOR * exif = reinterpret_cast<const AGAT_EXIF_SECTOR *>(data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR));
            if (exif->SIGNATURE[0] == 0xD6 && exif->SIGNATURE[1] == 0xD2) {
                // Agat images with an EXIF tail
                int mode_lo = exif->MODE & 0xF;
                int mode_hi = exif->MODE >> 4;
                if (mode_lo == 0) {
                    // Agat graphic modes
                    std::string type = "PICTURE_AGAT";
                    if (mode_hi == 1) return {type, "256x256x1"};
                    if (mode_hi == 4) return {type, "64x64x16"};
                    if (mode_hi == 5) return {type, "128x128x16"};
                    if (mode_hi == 6) return {type, "256x256x4"};
                    if (mode_hi == 7) return {type, "512x256x1"};
                    if (mode_hi == 8) return {type, "128x256x16"};
                } else
                if (mode_lo == 1) {
                    // Agat text modes
                    std::string type = "PICTURE_AGAT";
                    if (mode_hi == 2) return {type, "T32"};
                    if (mode_hi == 3) return {type, "T64"};
                } else
                if (mode_lo == 0xA) {
                    // Apple ][ graphic modes
                    std::string type = "PICTURE_APPLE";
                    if (mode_hi ==   9) return {type, "280x192HiRes"};
                    if (mode_hi == 0xC) return {type, "40x48LoRes"};
                    if (mode_hi == 0xD) return {type, "80x48DblLoRes"};
                    if (mode_hi == 0xE) return {type, "140x192DblHiRes"};
                    if (mode_hi == 0xF) return {type, "560x192DblHiResBW"};
                }
            }
        }
        return {"BINARY", ""};
    }

    std::string agat_vr_info(const BYTES & data)
    {
        std::string result = "";

        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            const AGAT_EXIF_SECTOR * exif = reinterpret_cast<const AGAT_EXIF_SECTOR*>(data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR));
            if (exif->SIGNATURE[0] == 0xD6 && exif->SIGNATURE[1] == 0xD2) {
                result += "{$AGAT_VR_FOUND}:\n";
                result += "    {$AGAT_VR_MODE}: $" + int_to_hex(exif->MODE) + " (";
                int mode_lo = exif->MODE & 0xF;
                int mode_hi = exif->MODE >> 4;
                result += std::string(agat_vr_mode_low[mode_lo]);
                switch (mode_lo) {
                case 0:
                    result += ", " + std::string(agat_vr_mode_high_agat_gr[mode_hi]);
                    break;
                case 1:
                    result += ", " + std::string(agat_vr_mode_high_agat_tx[mode_hi]);
                    break;
                case 10:
                    result += ", " + std::string(agat_vr_mode_high_apple[mode_hi]);
                    break;
                default:
                    break;
                }
                result += ")\n";
                if (mode_lo == 0 || mode_lo == 1) {
                    result += "    {$AGAT_VR_MAIN_PALETTE}: $" + int_to_hex(exif->PALETTE >> 4, false) + "\n";
                    result += "    {$AGAT_VR_ATL_PALETTE}: $" + int_to_hex(exif->PALETTE & 0xF, false) + "\n";
                    result += "    {$AGAT_VR_CUSTOM_PALETTE}:\n";
                    for (int i=0; i<8; i++) {

                        result += "        " + std::to_string(i*2) + ": #"
                            + int_to_hex((uint8_t)((exif->R[i] >> 4)*17))
                            + int_to_hex((uint8_t)((exif->G[i] >> 4)*17))
                            + int_to_hex((uint8_t)((exif->B[i] >> 4)*17))
                            + "\n";
                        result += "        " + std::to_string(i*2+1) + ": #"
                                  + int_to_hex((uint8_t)((exif->R[i] & 0xF)*17))
                                  + int_to_hex((uint8_t)((exif->G[i] & 0xF)*17))
                                  + int_to_hex((uint8_t)((exif->B[i] & 0xF)*17))
                                  + "\n";
                    }
                }
                if (mode_lo == 1) {
                    int font = exif->FONT >> 4;
                    result += "    {$AGAT_VR_FONT}: $" + int_to_hex(font, false) + " (" + std::string(agat_vr_font[font]) + ")\n";
                    if (font==15) {
                        result += "    {$AGAT_VR_CUSTOM_FONT}: " + trim(agat_to_utf(exif->FONT_NAME, 15)) + "\n";

                    }
                }
                std::string comment = "";
                for (int i=0; i<12; i++) {
                    std::string line = agat_to_utf(&(exif->COMMENT[i*16]), 16);
                    comment += line + "\n";
                }
                result += "    {$AGAT_VR_COMMENT}:\n";
                result += "----------------\n";
                result += trim(comment, " \t\n") + "\n";
                result += "----------------\n";
            }
            result += "\n";
        }

        return result;
    }

    void register_all_viewers() {
        dsk_tools::ViewerBinary viewer_binary;
        dsk_tools::ViewerText viewer_text;
        dsk_tools::ViewerBASIC_Agat viewer_basic_agat;
        dsk_tools::ViewerBASIC_Apple viewer_basic_apple;
        dsk_tools::ViewerBASIC_MBASIC viewer_basic_mbasic;
        dsk_tools::ViewerPicAgat_256x256x1 viewer_pic_agat_256x256x1;
        dsk_tools::ViewerPicAgat_512x256x1 viewer_pic_agat_512x256x1;
        dsk_tools::ViewerPicAgat_64x64x16 viewer_pic_agat_64x64x16;
        dsk_tools::ViewerPicAgat_128x128x16 viewer_pic_agat_128x128x16;
        dsk_tools::ViewerPicAgat_256x256x4 viewer_pic_agat_256x256x4;
        dsk_tools::ViewerPicAgat_128x256x16 viewer_pic_agat_128x256x16;

        dsk_tools::ViewerPicAgatTextT32 viewer_pic_agat_TextT32;
        dsk_tools::ViewerPicAgatTextT64 viewer_pic_agat_TextT64;

        dsk_tools::ViewerPicAgat_280x192HiRes viewer_pic_agat_280x192HiRes;
        dsk_tools::ViewerPicAgat_140x192DblHiRes viewer_pic_agat_140x192DblHiRes;
        dsk_tools::ViewerPicAgat_560x192DblHiResBW viewer_pic_agat_560x192DblHiResBW;
        dsk_tools::ViewerPicAgat_80x48DblLoRes viewer_pic_agat_80x48DblLoRes;
        dsk_tools::ViewerPicAgat_40x48LoRes viewer_pic_agat_40x48LoRes;

    }


} // namespace
