// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Track encodings (4-and-4, GCR 6-and-2, Agat MFM) and decoding of whole
// Agat 140/840 track images. Depends on nothing but the core files, see dsk_tools_core

#include "dsk_tools/core.h"

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

        // Ensure input size is even and sufficient for decoding
        if (in.size() < 2) {
            return;
        }

        for (size_t i = 0; i < in.size() / 2; i++)
        {
            // Bounds check: ensure we can read two bytes
            if (i * 2 + 1 >= in.size()) {
                break;
            }

            uint8_t b1 = in[i*2];
            uint8_t b2 = in[i*2+1];

            // Bounds check: ensure indices into decode table are valid (table size is 256)
            uint8_t idx1 = b1 >> 1;
            uint8_t idx2 = b2 >> 1;

            uint8_t b = (agat_MFM_decode_tab[idx1] << 4) | agat_MFM_decode_tab[idx2];
            out.push_back(b);
        }
    }

    // An Agat MFM track carries either 21 sectors of 256 bytes (840 Kb) or 11 of 512
    // (880 Kb, Nippel OS). The two are encoded identically apart from the length of a data
    // field, and nothing on the disk states it, so the checksum written after the field
    // decides: it only adds up for the length the disk was formatted with.
    int detect_agat_sector_size(const BYTES & in)
    {
        const int SECTOR_SIZE_840 = 256;
        const int SECTOR_SIZE_880 = 512;

        const int len = static_cast<int>(in.size());
        int matches_256 = 0;
        int matches_512 = 0;

        for (int p=0; p + 2 < len; p++) {
            // Data mark
            if (in[p] != 0x6A || in[p+1] != 0x95) continue;

            const int data_p = p + 2;
            if (data_p + SECTOR_SIZE_840 >= len) break;

            // The Agat checksum is an 8 bit sum with the carry added back
            uint16_t crc = 0;
            for (int i=0; i<SECTOR_SIZE_840; i++) {
                if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                crc += in[data_p + i];
            }
            if ((crc & 0xFF) == in[data_p + SECTOR_SIZE_840]) matches_256++;

            if (data_p + SECTOR_SIZE_880 >= len) continue;
            for (int i=SECTOR_SIZE_840; i<SECTOR_SIZE_880; i++) {
                if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                crc += in[data_p + i];
            }
            if ((crc & 0xFF) == in[data_p + SECTOR_SIZE_880]) matches_512++;
        }

        return (matches_512 > matches_256) ? SECTOR_SIZE_880 : SECTOR_SIZE_840;
    }

    Result decode_agat_840_track(BYTES &out, const BYTES & in, const int sectors, const int sector_size)
    {
        // A sector is: 95 6A | Volume Track Sector 5A | ... | 6A 95 | <sector_size> bytes | CRC 5A
        // An 840 Kb disk holds 21 sectors of 256 bytes, an 880 Kb one 11 sectors of 512
        const int ADDRESS_FIELD_LEN = 4;
        const int DATA_FIELD_LEN = sector_size;
        const int DATA_TAIL_LEN = 2;

        out.resize(sectors * sector_size);
        int track_len = in.size();
        int in_p = 0;
        bool errors = false;
        while (in_p < track_len) {
            // Looking for Index Mark
            bool index_found = false;
            while (in_p < track_len) {
                if (!iterate_until(in, in_p, 0x95)) break;
                if (in_p < track_len) {
                    uint8_t b1 = in[in_p++];
                    if (b1 == 0x6A) {index_found = true; break;};
                }
            }
            if (index_found) {
                // A truncated field means a damaged track, not a reason to read out of the buffer
                if (in_p + ADDRESS_FIELD_LEN > track_len) {errors = true; break;};

                uint8_t r_v = in[in_p++];
                uint8_t r_t = in[in_p++];
                uint8_t r_s = in[in_p++];
                // Index end mark
                uint8_t ie = in[in_p++];
                if (ie != 0x5A) errors = true;

                // Data mark
                bool data_found = false;
                while (in_p < track_len) {
                    if (!iterate_until(in, in_p, 0x6A)) break;
                    if (in_p < track_len) {
                        uint8_t b1 = in[in_p++];
                        if (b1 == 0x95) {data_found = true; break;};
                    }
                }
                if (data_found) {
                    // Data
                    int data_p = in_p;

                    if (in_p + DATA_FIELD_LEN + DATA_TAIL_LEN > track_len) {errors = true; break;};

                    uint16_t crc = 0;
                    for (int i=0; i<DATA_FIELD_LEN; i++) {
                        uint8_t  d = in[in_p++];
                        if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                        crc += d;
                    }
                    crc &= 0xFF;

                    uint8_t r_crc = in[in_p++];
                    if (r_crc != crc) errors = true;

                    if (r_s < sectors) {
                        int offset = r_s * sector_size;
                        std::copy(
                            in.begin() + data_p,
                            in.begin() + data_p + DATA_FIELD_LEN,
                            out.begin() + offset
                        );
                    }

                    // Data end mark
                    uint8_t de = in[in_p++];
                    if (de != 0x5A) errors = true;
                }
            }
        }
        if (!errors) {
            return Result::ok();
        } else {
            return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Agat 840 track decode error"));
        }
    }

    Result decode_agat_840_image(BYTES &out, const BYTES & in)
    {
        out.resize(160*21*256);
        int encoded_track_size = in.size() / 160;
        int raw_track_size = 21*256;
        for (int i=0; i<160; i++) {
            BYTES track_in(in.begin() + i*encoded_track_size, in.begin() + (i+1)*encoded_track_size);
            BYTES track_out;
            Result res = decode_agat_840_track(track_out, track_in);
            if (!res) return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Failed to decode Agat 840 track"));
            if (track_out.size() == raw_track_size) {
                std::copy(
                    track_out.begin(),
                    track_out.end(),
                    out.begin() + i*raw_track_size
                );
            } else  return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Decoded track size mismatch"));
        };
        return Result::ok();
    }

    Result load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len)
    {
        int in_p = 0;
        bool errors = false;
        while (in_p < track_len) {
            // Looking for Index Mark
            bool index_found = false;
            while (in_p < track_len) {
                if (!iterate_until(in, in_p, 0xD5)) break;
                if (in_p < track_len) {
                    uint8_t b1 = in.at(in_p++);
                    uint8_t b2 = in.at(in_p++);
                    if (b1 == 0xAA && b2 == 0x96) {index_found = true; break;};
                }
            }
            if (index_found) {
                BYTES ind_coded(in.begin() + in_p, in.begin() + in_p + 8);
                in_p += 8;
                BYTES ind = dsk_tools::decode44(ind_coded);
                uint8_t r_v = ind.at(0);
                uint8_t r_t = ind.at(1);
                uint8_t r_s = ind.at(2);
                uint8_t r_crc = ind.at(3);
                uint8_t expected_crc = static_cast<uint8_t>(r_v ^ r_t ^ r_s);
                if (r_crc != expected_crc || r_t != track) {
                    errors = true;
                }
                // Index end mark
                BYTES ie(in.begin()+in_p, in.begin()+in_p + 3); in_p += 3;
                if (ie.at(0) != 0xDE || ie.at(1) != 0xAA || ie.at(2) != 0xEB) {
                    errors = true;
                }

                // Data mark
                bool data_found = false;
                while (in_p < track_len) {
                    if (!iterate_until(in, in_p, 0xD5)) break;
                    if (in_p < track_len) {
                        uint8_t b1 = in.at(in_p++);
                        uint8_t b2 = in.at(in_p++);
                        if (b1 == 0xAA && b2 == 0xAD) {data_found = true; break;};
                    }
                };
                if (data_found) {
                    // Data
                    BYTES encoded_sector(in.begin()+ in_p, in.begin()+ in_p + 343);
                    in_p += 343;
                    BYTES data(256);
                    bool crc_ok = decode_gcr62(encoded_sector.data(), data.data());
                    if (!crc_ok) {
                        errors = true;
                    }
                    // Data end mark
                    BYTES de(in.begin()+in_p, in.begin()+in_p + 3); in_p += 3;
                    if (de.at(0) != 0xDE || de.at(1) != 0xAA || de.at(2) != 0xEB) {
                        errors = true;
                    }
                    if (r_s < 16) {
                        int t_s = agat_140_raw2logic[r_s];
                        std::copy(data.begin(), data.end(), buffer.begin() + (track*16 + t_s)*256);
                    }
                }
            }
        }

        if (!errors) {
            return Result::ok();
        } else {
            return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Agat 140 track decode error"));
        }

    }

    Result decode_agat_140_image(BYTES &out, const BYTES & in, const int track_len)
    {
        out.resize(35*16*256);
        for (int i=0; i<35; i++) {
            BYTES track_in(in.begin() + i*track_len, in.begin() + (i+1)*track_len);
            Result res = load_agat140_track(i, out, track_in, track_len);
            if (!res) return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Failed to decode Agat 140 track"));
        };
        return Result::ok();
    }

} // namespace
