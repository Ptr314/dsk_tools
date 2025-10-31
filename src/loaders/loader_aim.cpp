// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for AIM (Agat 840 Kb psysical images)

#include <fstream>
#include <iostream>

#include "loader_aim.h"
#include "utils.h"

namespace dsk_tools {
LoaderAIM::LoaderAIM(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    bool LoaderAIM::iterate_until(const std::vector<uint16_t> & in, int & p, const uint8_t v)
    {
        uint16_t b;
        uint8_t d;
        do {
            if (p >= in.size()) return false;
            b = in.at(p++);
            d = b & 0xFF;
        } while (d != v);
        return true;
    }

    int LoaderAIM::load(BYTES & buffer)
    {
        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

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
                    if (in_p > in.size()) return FDD_LOAD_ERROR;
                    uint16_t b = in.at(in_p++);
                    uint8_t  d = b & 0xFF;
                    buffer[out_p++] = d;
                }
            }
        }

        loaded = true;

        return FDD_LOAD_OK;
    }

    std::string LoaderAIM::file_info()
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


        std::vector<uint16_t> in(fsize/2);

        file.read (reinterpret_cast<char*>(in.data()), fsize);

        int in_p = 0;
        int track_len = 6464;
        result += "\n";
        bool error = false;
        bool errors = false;
        for (int track=0; track<160; track++) {
            int in_base = track * track_len;
            in_p = in_base;
            result += "{$TRACK}: " + std::to_string(track) + "\n";
            while (in_p < in_base+track_len) {
                // Looking for Index Mark
                bool index_found = false;
                while (in_p < in_base+track_len) {
                    if (!iterate_until(in, in_p, 0x95)) break;
                    if (in_p < in_base+track_len) {
                        uint8_t b = in.at(in_p++) & 0xFF;
                        if (b == 0x6A) {index_found = true; break;};
                    }
                }
                if (index_found) {
                    // VTS
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p - 2)) + " {$INDEX_MARK} ($95 $6A)\n";
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p)) + " {$SECTOR_INDEX}:";
                    uint8_t r_v = in.at(in_p++) & 0xFF;
                    uint8_t r_t = in.at(in_p++) & 0xFF;
                    uint8_t r_s = in.at(in_p++) & 0xFF;
                    uint8_t r_e = in.at(in_p++) & 0xFF;
                    result += " {$VOLUME_ID}=" + std::to_string(r_v) + " ($" + dsk_tools::int_to_hex(r_v) + ")";
                    result += ", {$TRACK_SHORT}=" + std::to_string(r_t);
                    result += ", {$LOGICAL_SECTOR}=" + std::to_string(r_s);
                    if (r_e == 0x5A) {
                        result += ", {$SECTOR_INDEX_END_OK}";
                    } else {
                        result += ", {$SECTOR_INDEX_END_ERROR}";
                    }
                    result += "\n";
                    // Data mark
                    bool data_found = false;
                    while (in_p < in_base+track_len) {
                        if (!iterate_until(in, in_p, 0x6A)) break;
                        uint8_t b = in.at(in_p++) & 0xFF;
                        if (b == 0x95) {data_found = true; break;};
                    };
                    if (data_found) {
                        // Data
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p-2)) + " {$DATA_MARK} ($6A $95)\n";
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p)) + " {$DATA_FIELD} (256)\n";
                        uint16_t crc = 0;
                        error = false;
                        for (int i=0; i<256; i++) {
                            if (in_p >= in_base+track_len) {error = true; break;};
                            uint8_t  d = in.at(in_p++) & 0xFF;
                            if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                            crc += d;
                        }
                        if (!error) {
                            result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p));
                            uint8_t r_crc = in.at(in_p++) & 0xFF;
                            if (r_crc == (crc & 0xFF)) {
                                result += " {$SECTOR_CRC_OK} ($" + dsk_tools::int_to_hex(r_crc) + ")";
                            } else {
                                errors = true;
                                result += " {$SECTOR_CRC_ERROR} ({$CRC_EXPECTED}: $" + dsk_tools::int_to_hex(static_cast<uint8_t>(crc & 0xFF)) + ", {$CRC_FOUND}: $" + dsk_tools::int_to_hex(r_crc) + ")";
                            }
                        } else {
                            result += " {$SECTOR_ERROR}";
                            errors = true;
                        }
                        result += "\n\n";
                    }
                }
            }
            for (int i=0; i<track_len; i++) {
                uint8_t hi = in.at(i) >> 8;
                if (hi != 0) {
                    result += "    Command[$" + dsk_tools::int_to_hex(static_cast<uint16_t>(i)) + "] = $" + dsk_tools::int_to_hex(hi) + "\n";
                }
            }
        }
        if (errors) {
            result += "{$ERROR_PARSING}\n";
        } else {
            result += "{$PARSING_FINISHED}\n";
        }

        return result;
    }
}
