// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A top level abstract class for different physical format loaders

#include <fstream>
#include <iostream>

#include "loader_mfm.h"
#include "dsk_tools/dsk_tools.h"
#include "utils.h"

namespace dsk_tools {
    LoaderMFM::LoaderMFM(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {
        if (type_id == "TYPE_AGAT_140") {
            track_info_func = &LoaderMFM::agat140_track_info;
            load_track_func = &LoaderMFM::load_agat140_track;
        } else
            if (type_id == "TYPE_AGAT_840") {
                track_info_func = &LoaderMFM::agat840_track_info;
                load_track_func = &LoaderMFM::load_agat840_track;
            } else
                throw std::runtime_error("LoaderMFM: Incorrect type id");
    }

    void LoaderMFM::prepare_tracks_list(BYTES & in)
    {
        for (int track=0; track < get_tracks_count(); track ++) {
            m_track_offsets[track] = track * m_track_len;
            m_track_lengths[track] = m_track_len;
        }
    }

    std::string LoaderMFM::agat140_track_info(BYTES & in, int track_len)
    {
        std::string result;
        bool errors = false;
        int in_p = 0;

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
                result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p - 3)) + " {$INDEX_MARK} ($D5 $AA $96)\n";
                result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p)) + " {$SECTOR_INDEX}:";
                BYTES ind_coded(in.begin() + in_p, in.begin() + in_p + 8);
                in_p += 8;
                BYTES ind = dsk_tools::decode44(ind_coded);
                uint8_t r_v = ind.at(0);
                uint8_t r_t = ind.at(1);
                uint8_t r_s = ind.at(2);
                uint8_t r_crc = ind.at(3);
                uint8_t expecter_crc = static_cast<uint8_t>(r_v ^ r_t ^ r_s);
                result += " {$VOLUME_ID}=" + std::to_string(r_v) + " ($" + dsk_tools::int_to_hex(r_v) + ")";
                result += ", {$TRACK_SHORT}=" + std::to_string(r_t);
                result += ", {$LOGICAL_SECTOR}=" + std::to_string(r_s);
                if (r_crc == expecter_crc) {
                    result += ", {$INDEX_CRC_OK}";
                } else {
                    errors = true;
                    result += " {$INDEX_CRC_ERROR} ({$CRC_EXPECTED}: $" + dsk_tools::int_to_hex(static_cast<uint8_t>(expecter_crc)) + ", {$CRC_FOUND}: $" + dsk_tools::int_to_hex(r_crc) + ")";
                }
                // Index end mark
                BYTES ie(in.begin()+in_p, in.begin()+in_p + 3); in_p += 3;
                if (ie.at(0) == 0xDE && ie.at(1) == 0xAA && ie.at(2) == 0xEB) {
                    result += ", {$INDEX_EPILOGUE_OK}";
                } else {
                    errors = true;
                    result += ", {$INDEX_EPILOGUE_ERROR}";
                }
                result += "\n";

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
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p-3)) + " {$DATA_MARK} ($D5 $AA $AD)\n";
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p)) + " {$DATA_FIELD} (342+1)";
                    if (in_p + 343 + 3 <= in.size()) {
                        BYTES encoded_sector(in.begin()+ in_p, in.begin()+ in_p + 343);
                        in_p += 343;
                        BYTES data(256);
                        bool crc_ok = decode_gcr62(encoded_sector.data(), data.data());
                        if (crc_ok) {
                            result += ", {$SECTOR_CRC_OK}";
                        } else {
                            errors = true;
                            result += ", {$SECTOR_CRC_ERROR}";
                        }
                        uint8_t e1 = in.at(in_p++);
                        uint8_t e2 = in.at(in_p++);
                        uint8_t e3 = in.at(in_p++);
                        if (e1 == 0xDE && e2 == 0xAA && e3 == 0xEB) {
                            result += ", {$DATA_EPILOGUE_OK}";
                        } else {
                            errors = true;
                            result += ", {$DATA_EPILOGUE_ERROR}";
                        }

                        result += "\n    " + dsk_tools::toHexList(data.data(), 16) + " ...";
                    } else {
                        errors = true;
                    }

                    result += "\n\n";
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

    std::string LoaderMFM::agat840_track_info(BYTES & in, int track_len)
    {
        std::string result;
        int in_p = 0;
        bool errors = false;
        while (in_p < track_len) {
            // Looking for Index Mark
            bool index_found = false;
            while (in_p < track_len) {
                if (!iterate_until(in, in_p, 0x95)) break;
                if (in_p < track_len) {
                    uint8_t b1 = in.at(in_p++);
                    if (b1 == 0x6A) {index_found = true; break;};
                }
            }
            if (index_found) {
                result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p-2)) + " {$INDEX_MARK} ($95 $6A)\n";
                result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p)) + " {$SECTOR_INDEX}:";
                uint8_t r_v = in.at(in_p++);
                uint8_t r_t = in.at(in_p++);
                uint8_t r_s = in.at(in_p++);
                result += " {$VOLUME_ID}=" + std::to_string(r_v) + " ($" + dsk_tools::int_to_hex(r_v) + ")";
                result += ", {$TRACK_SHORT}=" + std::to_string(r_t);
                result += ", {$LOGICAL_SECTOR}=" + std::to_string(r_s);
                // Index end mark
                uint8_t ie = in.at(in_p++);
                if (ie == 0x5A) {
                    result += ", {$INDEX_EPILOGUE_OK}";
                } else {
                    errors = true;
                    result += ", {$INDEX_EPILOGUE_ERROR}";
                }
                result += "\n";
                // Data mark
                bool data_found = false;
                while (in_p < track_len) {
                    if (!iterate_until(in, in_p, 0x6A)) break;
                    if (in_p < track_len) {
                        uint8_t b1 = in.at(in_p++);
                        if (b1 == 0x95) {data_found = true; break;};
                    }
                }
                if (data_found) {
                    // Data
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p-2)) + " {$DATA_MARK} ($6A $95)\n";
                    result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p)) + " {$DATA_FIELD} (256)\n";

                    bool error = false;
                    uint16_t crc = 0;
                    int data_p = in_p;
                    result += "               " + dsk_tools::toHexList(in.data() + data_p, 16) + " ...\n";

                    for (int i=0; i<256; i++) {
                        if (in_p >= track_len) {error = true; break;};
                        uint8_t  d = in.at(in_p++);
                        if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                        crc += d;
                    }
                    crc &= 0xFF;
                    if (!error) {
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p));
                        uint8_t r_crc = in.at(in_p++);
                        if (r_crc == crc) {
                            result += " {$SECTOR_CRC_OK} ($" + dsk_tools::int_to_hex(r_crc) + ")";
                        } else {
                            errors = true;
                            result += " {$SECTOR_CRC_ERROR} ({$CRC_EXPECTED}: $" + dsk_tools::int_to_hex(static_cast<uint8_t>(crc)) + ", {$CRC_FOUND}: $" + dsk_tools::int_to_hex(r_crc) + ")";
                        }
                        // Data end mark
                        uint8_t de = in.at(in_p++);
                        if (de == 0x5A) {
                            result += ", {$DATA_EPILOGUE_OK}";
                        } else {
                            errors = true;
                            result += ", {$DATA_EPILOGUE_ERROR}";
                        }

                        result += "\n";

                    } else {
                        result += " {$SECTOR_ERROR}";
                        errors = true;
                    }
                    result += "\n";
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

    int LoaderMFM::load(std::vector<uint8_t> &buffer)
    {
        std::ifstream file(file_name, std::ios::binary);
        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        int image_size = get_tracks_count()*get_sectors_count()*256;
        buffer.resize(image_size);

        BYTES in_all(fsize);
        bool errors = false;

        file.read (reinterpret_cast<char*>(in_all.data()), fsize);

        prepare_tracks_list(in_all);

        for (int track=0; track<get_tracks_count(); track++) {
            int in_base = get_track_offset(track);
            int track_len = get_track_len(track);
            BYTES in(in_all.begin()+in_base, in_all.begin()+in_base+track_len);
            (this->*load_track_func)(track, buffer, in, track_len);
        }
        loaded = true;
        return FDD_LOAD_OK;
    }

    std::string LoaderMFM::file_info()
    {
        std::string result = "";

        std::ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}\n";
            return result;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(fsize) + " {$BYTES}\n";
        result += "\n";

        BYTES in_all(fsize);
        file.read (reinterpret_cast<char*>(in_all.data()), fsize);

        prepare_tracks_list(in_all);

        result += get_header_info(in_all);

        for (int track=0; track<get_tracks_count(); track++) {
            int in_base = get_track_offset(track);
            int track_len = get_track_len(track);
            BYTES in(in_all.begin()+in_base, in_all.begin()+in_base+track_len);
            result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_base)) + ": {$TRACK} " + std::to_string(track) + "\n";

            result += (this->*track_info_func)(in, track_len);

            result += "\n";
        }

        return result;
    }


    void LoaderMFM::load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len)
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
                        // int t_s = r_s;
                        std::copy(data.begin(), data.end(), buffer.begin() + (track*16 + t_s)*256);
                    }
                }
            }
        }
    }

    void LoaderMFM::load_agat840_track(int track, BYTES & buffer, const BYTES & in, int track_len)
    {
        int in_p = 0;
        bool errors = false;

        while (in_p < track_len) {
            // Looking for Index Mark
            bool index_found = false;
            while (in_p < track_len) {
                if (!iterate_until(in, in_p, 0x95)) break;
                if (in_p < track_len) {
                    uint8_t b1 = in.at(in_p++);
                    if (b1 == 0x6A) {index_found = true; break;};
                }
            }
            if (index_found) {
                uint8_t r_v = in.at(in_p++);
                uint8_t r_t = in.at(in_p++);
                uint8_t r_s = in.at(in_p++);
                // Index end mark
                uint8_t ie = in.at(in_p++);
                if (ie != 0x5A) errors = true;

                // Data mark
                bool data_found = false;
                while (in_p < track_len) {
                    if (!iterate_until(in, in_p, 0x6A)) break;
                    if (in_p < track_len) {
                        uint8_t b1 = in.at(in_p++);
                        if (b1 == 0x95) {data_found = true; break;};
                    }
                }
                if (data_found) {
                    // Data
                    bool error = false;
                    uint16_t crc = 0;
                    int data_p = in_p;

                    for (int i=0; i<256; i++) {
                        if (in_p >= track_len) {error = true; break;};
                        uint8_t  d = in.at(in_p++);
                        if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                        crc += d;
                    }
                    crc &= 0xFF;
                    if (!error) {
                        uint8_t r_crc = in.at(in_p++);
                        if (r_crc != crc) errors = true;

                        if (r_s < 21) {
                            int offset = (track  * 21 + r_s) * 256;
                            std::copy(
                                in.begin() + data_p,
                                in.begin() + data_p + 256,
                                buffer.begin() + offset
                                // buffer.begin() + (((track << 1) + s) * 21 + r_s) * 256
                                );
                        }

                        // Data end mark
                        uint8_t de = in.at(in_p++);
                        if (de != 0x5A) errors = true;

                    } else {
                        errors = true;
                    }
                }
            }
        }
    }

}
