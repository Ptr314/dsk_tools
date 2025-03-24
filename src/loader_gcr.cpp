#include <fstream>
#include <iostream>

#include "dsk_tools/dsk_tools.h"

namespace dsk_tools {
LoaderGCR::LoaderGCR(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    int LoaderGCR::get_track_offset(int track)
    {
        return track * get_track_len(track);
    }


    int LoaderGCR::load(std::vector<uint8_t> &buffer)
    {
        std::ifstream file(file_name, std::ios::binary);
        if (!file.good()) {
            return FDD_LOAD_ERROR;
        }

        file.seekg (0, file.end);
        auto fsize = file.tellg();
        file.seekg (0, file.beg);

        int image_size = 35*16*256;
        buffer.resize(image_size);

        BYTES in(fsize);
        bool errors = false;

        file.read (reinterpret_cast<char*>(in.data()), fsize);

        prepare_tracks_list(in);

        for (int track=0; track<35; track++) {
            int in_base = get_track_offset(track);
            int track_len = get_track_len(track);
            int in_p = in_base;

            while (in_p < in_base+track_len) {
                // Looking for Index Mark
                bool index_found = false;
                while (in_p < in_base+track_len) {
                    if (!iterate_until(in, in_p, 0xD5)) break;
                    if (in_p < in_base+track_len) {
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
                    uint8_t expecter_crc = static_cast<uint8_t>(r_v ^ r_t ^ r_s);
                    if (r_crc != expecter_crc || r_t != track) {
                        errors = true;
                    }
                    // Index end mark
                    BYTES ie(in.begin()+in_p, in.begin()+in_p + 3); in_p += 3;
                    if (ie.at(0) != 0xDE || ie.at(1) != 0xAA || ie.at(2) != 0xEB) {
                        errors = true;
                    }

                    // Data mark
                    bool data_found = false;
                    while (in_p < in_base+track_len) {
                        if (!iterate_until(in, in_p, 0xD5)) break;
                        if (in_p < in_base+track_len) {
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
                            // int offset = r_s * 256;
                            int t_s = agat_140_raw2logic[r_s];
                            // int t_s = r_s;
                            std::copy(data.begin(), data.end(), buffer.begin() + (track*16 + t_s)*256);
                        }
                    }
                }
            }
        }

        loaded = true;
        return FDD_LOAD_OK;
    }

    std::string LoaderGCR::file_info()
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
        file.read (reinterpret_cast<char*>(in.data()), fsize);

        bool errors = false;

        prepare_tracks_list(in);

        if (format_id == "FILE_HXC_MFM") {
            HXC_MFM_HEADER * hdr;

            hdr = reinterpret_cast<HXC_MFM_HEADER *>(in.data());
            result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(0)) + " {$HEADER}\n";

            result += "    {$TRACKS}: " + std::to_string(hdr->number_of_track) + "\n";
            result += "    {$SIDES}: " + std::to_string(hdr->number_of_side) + "\n";
            result += "$" + dsk_tools::int_to_hex(hdr->mfmtracklistoffset) + " {$TRACKLIST_OFFSET}\n";

            for (int track=0; track<hdr->number_of_track; track++) {
                HXC_MFM_TRACK_INFO * ti = reinterpret_cast<HXC_MFM_TRACK_INFO *>(in.data() + hdr->mfmtracklistoffset + track * sizeof(HXC_MFM_TRACK_INFO));
                result += "    {$SIDE_SHORT}: " + std::to_string(ti->side_number)
                          + ", {$TRACK_SHORT}=" + std::to_string(ti->track_number)
                          + ", {$TRACK_OFFSET}: $" + dsk_tools::int_to_hex(ti->mfmtrackoffset, false)
                          + ", {$TRACK_SIZE}: $" + dsk_tools::int_to_hex(ti->mfmtracksize, false)
                          + "\n";
            }
            result += "\n";
        };

        int in_p = 0;
        uint8_t e1, e2, e3;

        for (int track=0; track<35; track++) {
            int in_base = get_track_offset(track);
            int track_len = get_track_len(track);
            int in_p = in_base;

            result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_base)) + ": {$TRACK} " + std::to_string(track) + "\n";
            while (in_p < in_base+track_len) {
                // Looking for Index Mark
                bool index_found = false;
                while (in_p < in_base+track_len) {
                    if (!iterate_until(in, in_p, 0xD5)) break;
                    if (in_p < in_base+track_len) {
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
                    while (in_p < in_base+track_len) {
                        if (!iterate_until(in, in_p, 0xD5)) break;
                        if (in_p < in_base+track_len) {
                            uint8_t b1 = in.at(in_p++);
                            uint8_t b2 = in.at(in_p++);
                            if (b1 == 0xAA && b2 == 0xAD) {data_found = true; break;};
                        }
                    };
                    if (data_found) {
                        // Data
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p-3)) + " {$DATA_MARK} ($D5 $AA $AD)\n";
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_p)) + " {$DATA_FIELD} (342+1)";
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
                        e1 = in.at(in_p++);
                        e2 = in.at(in_p++);
                        e3 = in.at(in_p++);
                        if (e1 == 0xDE && e2 == 0xAA && e3 == 0xEB) {
                            result += ", {$DATA_EPILOGUE_OK}";
                        } else {
                            errors = true;
                            result += ", {$DATA_EPILOGUE_ERROR}";
                        }

                        result += "\n    " + dsk_tools::toHexList(data.data(), 16) + " ...";

                        result += "\n\n";
                    }
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
