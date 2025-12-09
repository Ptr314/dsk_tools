// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .HFE files

#include <cmath>
#include <fstream>
#include <iostream>

#include "host_helpers.h"

#include "dsk_tools/dsk_tools.h"
#include "definitions.h"
#include "utils.h"
#include "loader_hxc_hfe.h"

namespace dsk_tools {
LoaderHXC_HFE::LoaderHXC_HFE(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    Result LoaderHXC_HFE::load(BYTES &buffer)
    {
        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return Result::error(ErrorCode::LoadError, "Cannot open file");
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

        BYTES in(fsize);
        bool errors = false;

        file.read (reinterpret_cast<char*>(in.data()), fsize);

        HXC_HFE_HEADER * hdr = reinterpret_cast<HXC_HFE_HEADER*>(in.data());

        std::string signature(reinterpret_cast<char*>(&hdr->HEADERSIGNATURE), sizeof(hdr->HEADERSIGNATURE));

        if (signature != "HXCPICFE") return Result::error(ErrorCode::LoadIncorrectFile, "Invalid HFE signature");

        int image_size, sectors_per_track, sector_size;

        if (type_id == "TYPE_AGAT_840") {
            if (hdr->number_of_side != 2 || hdr->number_of_track != 80)
                return Result::error(ErrorCode::LoadIncorrectFile, "Invalid HFE parameters");
            sectors_per_track = 21;
            sector_size = 256;
            image_size = 2*80*sectors_per_track*sector_size;
        } else
            return Result::error(ErrorCode::LoadIncorrectFile, "Unsupported disk type");

        buffer.resize(image_size);


        int tracklist_offset = hdr->track_list_offset*HXC_HFE_BLOCK_SIZE;

        std::vector<HXC_HFE_TRACK*> ti(hdr->number_of_track);

        for (int track=0; track < hdr->number_of_track; track++)
            ti[track] = reinterpret_cast<HXC_HFE_TRACK *>(in.data() + tracklist_offset + track * sizeof(HXC_HFE_TRACK));

        for (int track=0; track<hdr->number_of_track; track++) {
            int in_base = ti[track]->offset*HXC_HFE_BLOCK_SIZE;
            // TODO: remove float operations, use (a + b - 1) / b
            int mixed_track_len = std::ceil(static_cast<double>(ti[track]->track_len) / HXC_HFE_BLOCK_SIZE) * HXC_HFE_BLOCK_SIZE;

            BYTES track_mixed(in.begin() + in_base, in.begin() + in_base + mixed_track_len);
            std::vector<BYTES> track_mfm(hdr->number_of_side);
            if (hdr->number_of_side == 1) {
                track_mfm[0] = track_mixed;
            } else {
                for (int i=0; i < mixed_track_len / HXC_HFE_BLOCK_SIZE; i++)
                    for (int s=0; s < hdr->number_of_side; s++ ) {
                        auto part_base = track_mixed.begin() + i*HXC_HFE_BLOCK_SIZE + s*(HXC_HFE_BLOCK_SIZE/2);
                        track_mfm[s].insert(track_mfm[s].end(), part_base, part_base + HXC_HFE_BLOCK_SIZE / 2);
                    }
            }

            for (int s=0; s < hdr->number_of_side; s++ ) {
                BYTES track_data;
                decode_agat_mfm_data(track_data, track_mfm[s]);

                BYTES raw_data;
                if (decode_agat_840_track(raw_data, track_data) == FDD_LOAD_OK){
                    std::copy(
                        raw_data.begin(),
                        raw_data.end(),
                        buffer.begin() + (((track << 1) + s) * sectors_per_track) * sector_size
                    );
                } else
                    errors = true;
            }
        }
        loaded = true;
        if (!errors) {
            return Result::ok();
        } else {
            return Result::error(ErrorCode::LoadDataCorrupt, "Failed to decode track data");
        }
    }

    std::string LoaderHXC_HFE::file_info()
    {
        std::string result = "";

        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}:\n";
            return result;
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

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
            result += "    {$TRACKLIST_OFFSET}: " + std::to_string(hdr->track_list_offset) + "*512 ($" + dsk_tools::int_to_hex(hdr->track_list_offset*HXC_HFE_BLOCK_SIZE, false) + ")\n";
        } else {
            result += "\n{$NO_SIGNATURE}\n";
            return result;
        }

        int tracklist_offset = hdr->track_list_offset*HXC_HFE_BLOCK_SIZE;

        result += "$" + dsk_tools::int_to_hex(tracklist_offset) + " {$TRACKLIST_OFFSET}\n";

        std::vector<HXC_HFE_TRACK*> ti(hdr->number_of_track);

        for (int track=0; track < hdr->number_of_track; track++) {
            ti[track] = reinterpret_cast<HXC_HFE_TRACK *>(in.data() + tracklist_offset + track * sizeof(HXC_HFE_TRACK));
            result += "    " + std::to_string(track) + ":"
                      + " {$TRACK_OFFSET}: $" + dsk_tools::int_to_hex(ti[track]->offset*HXC_HFE_BLOCK_SIZE, false)
                      + ", {$TRACK_SIZE}: $" + dsk_tools::int_to_hex(ti[track]->track_len, false)
                      + "\n";
        }
        result += "\n";

        for (int track=0; track<hdr->number_of_track; track++) {
            int in_base = ti[track]->offset*HXC_HFE_BLOCK_SIZE;
            int mixed_track_len = std::ceil(static_cast<double>(ti[track]->track_len) / HXC_HFE_BLOCK_SIZE) * HXC_HFE_BLOCK_SIZE;

            // result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_base)) + ": {$TRACK} " + std::to_string(track) + "\n";
            BYTES track_mixed(in.begin() + in_base, in.begin() + in_base + mixed_track_len);
            std::vector<BYTES> track_mfm(hdr->number_of_side);
            if (hdr->number_of_side == 1) {
                track_mfm[0] = track_mixed;
            } else {
                for (int i=0; i < mixed_track_len / HXC_HFE_BLOCK_SIZE; i++)
                    for (int s=0; s < hdr->number_of_side; s++ ) {
                        auto part_base = track_mixed.begin() + i*HXC_HFE_BLOCK_SIZE + s*(HXC_HFE_BLOCK_SIZE/2);
                        track_mfm[s].insert(track_mfm[s].end(), part_base, part_base + HXC_HFE_BLOCK_SIZE / 2);
                    }
            }

            for (int s=0; s < hdr->number_of_side; s++ ) {
                BYTES track_data;
                decode_agat_mfm_data(track_data, track_mfm[s]);
                int track_len = track_data.size();
                result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(in_base)) + ": {$TRACK} " + std::to_string(track) + " {$SIDE} " + std::to_string(s) + "\n";
                int in_p = 0;
                while (in_p < track_len) {
                    // Looking for Index Mark
                    bool index_found = false;
                    while (in_p < track_len) {
                        if (!iterate_until(track_data, in_p, 0x95)) break;
                        if (in_p < track_len) {
                            uint8_t b1 = track_data.at(in_p++);
                            if (b1 == 0x6A) {index_found = true; break;};
                        }
                    }
                    if (index_found) {
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p-2)) + " {$INDEX_MARK} ($95 $6A)\n";
                        result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p)) + " {$SECTOR_INDEX}:";
                        uint8_t r_v = track_data.at(in_p++);
                        uint8_t r_t = track_data.at(in_p++);
                        uint8_t r_s = track_data.at(in_p++);
                        result += " {$VOLUME_ID}=" + std::to_string(r_v) + " ($" + dsk_tools::int_to_hex(r_v) + ")";
                        result += ", {$TRACK_SHORT}=" + std::to_string(r_t);
                        result += ", {$LOGICAL_SECTOR}=" + std::to_string(r_s);
                        // Index end mark
                        uint8_t ie = track_data.at(in_p++);
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
                            if (!iterate_until(track_data, in_p, 0x6A)) break;
                            if (in_p < track_len) {
                                uint8_t b1 = track_data.at(in_p++);
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
                            result += "               " + dsk_tools::toHexList(track_data.data() + data_p, 16) + " ...\n";

                            for (int i=0; i<256; i++) {
                                if (in_p >= track_len) {error = true; break;};
                                uint8_t  d = track_data.at(in_p++);
                                if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                                crc += d;
                            }
                            crc &= 0xFF;
                            if (!error) {
                                result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p));
                                uint8_t r_crc = track_data.at(in_p++);
                                if (r_crc == crc) {
                                    result += " {$SECTOR_CRC_OK} ($" + dsk_tools::int_to_hex(r_crc) + ")";
                                } else {
                                    errors = true;
                                    result += " {$SECTOR_CRC_ERROR} ({$CRC_EXPECTED}: $" + dsk_tools::int_to_hex(static_cast<uint8_t>(crc)) + ", {$CRC_FOUND}: $" + dsk_tools::int_to_hex(r_crc) + ")";
                                }
                                // Data end mark
                                uint8_t de = track_data.at(in_p++);
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

