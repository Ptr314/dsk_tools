// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .IMD files

#include "host_helpers.h"

#include "loader_imd.h"
#include "dsk_tools/dsk_tools.h"
#include "utils.h"

namespace dsk_tools {

    LoaderIMD::LoaderIMD(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    Result LoaderIMD::load(BYTES &buffer)
    {
        const UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return Result::error(ErrorCode::LoadError, "Cannot open file");
        }

        // TODO: Load here

        loaded = true;

        return Result::ok();
    }

    std::string LoaderIMD::file_info()
    {
        std::string result = "";

        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            result += "{$ERROR_OPENING}\n";
            return result;
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(fsize) + " {$BYTES}\n";

        constexpr unsigned signature_length = 29;
        char header[signature_length];
        file.read(header, signature_length);

        if (std::string(header, 3) != "IMD") {
            result += "{$ERROR}: {$INVALID_SIGNATURE}\n";
            return result;
        }

        result += "{$SIGNATURE}: " + std::string(header, signature_length) + "\n";

        std::string comment;
        char c;
        while (file.read(&c, 1) && c != 0x1A) {
            comment += c;
        }
        if (c != 0x1A) {
            result += "{$ERROR}: {$UNEXPECTED_EOF}\n";
            return result;
        }
        result += "{$COMMENT}: " + trim(comment) + "\n";
        result += "\n";

        while (file.good()) {
            // Track header
            IMD_TRACK_HEADER track_header{};
            file.read(reinterpret_cast<char*>(&track_header), sizeof(IMD_TRACK_HEADER));
            if (!file.good()) {
                // result += "{$UNEXPECTED_END_OF_FILE}\n";
                return result;
            }
            int sector_size = 1 << (track_header.sector_size+7);
            result += "{$TRACK}: " + std::to_string(track_header.cylinder)
                        + ", {$SIDE}: " + std::to_string(track_header.head & 0x3F)
                        + ", {$CPM_SECTORS}: " + std::to_string(track_header.sectors)
                        + ", {$CPM_SECTOR_SIZE}: " + std::to_string(sector_size)
                        +"\n";

            // Sector map
            BYTES sector_map;
            sector_map.resize(track_header.sectors);
            file.read(reinterpret_cast<char*>(sector_map.data()), sector_map.size());
            if (!file.good()) {
                result += "{$UNEXPECTED_END_OF_FILE}\n";
                return result;
            }
            result += "    {$SECTORS_MAP}: " + toHexList(sector_map.data(), sector_map.size()) + "\n";

            //Cylinder map
            static bool cm_presents = (track_header.head & 0x80) != 0;
            if (cm_presents) {
                BYTES cylinder_map;
                cylinder_map.resize(track_header.sectors);
                file.read(reinterpret_cast<char*>(cylinder_map.data()), cylinder_map.size());
                if (!file.good()) {
                    result += "{$UNEXPECTED_END_OF_FILE}\n";
                    return result;
                }
                result += "    {$CYLINDERS_MAP}: " + toHexList(cylinder_map.data(), cylinder_map.size()) + "\n";
            }

            //Head map
            static bool hm_presents = (track_header.head & 0x40) != 0;
            if (hm_presents) {
                BYTES head_map;
                head_map.resize(track_header.sectors);
                file.read(reinterpret_cast<char*>(head_map.data()), head_map.size());
                if (!file.good()) {
                    result += "{$UNEXPECTED_END_OF_FILE}\n";
                    return result;
                }
                result += "    {$HEAD_MAP}: " + toHexList(head_map.data(), head_map.size()) + "\n";
            }

            for (unsigned sector=0; sector<track_header.sectors; sector++) {
                result += "    " + int_to_hex(static_cast<uint8_t>(sector+1)) + " (" + int_to_hex(sector_map[sector]) + "): ";

                uint8_t data_marker = 0;

                file.read(reinterpret_cast<char*>(&data_marker), 1);

                int skip=-1;

                switch (data_marker) {
                    case 0x00:
                        result += "{$SECTOR_UNAVAILABLE}";
                        skip = 0;
                        break;
                    case 0x01:
                        result += "{$NORMAL_DATA}";
                        skip = sector_size;
                        break;
                    case 0x02:
                        result += "{$NORMAL_DATA}, {$DATA_COMPRESSED}";
                        skip = 1;
                        break;
                    case 0x03:
                        result += "{$NORMAL_DATA}, {$DATA_DELETED}";
                        skip = sector_size;
                        break;
                    case 0x04:
                        result += "{$NORMAL_DATA}, {$DATA_COMPRESSED}, {$DATA_DELETED}";
                        skip = 1;
                        break;
                    case 0x05:
                        result += "{$NORMAL_DATA_WITH_ERROR}";
                        skip = sector_size;
                        break;
                    case 0x06:
                        result += "{$NORMAL_DATA_WITH_ERROR}, {$DATA_COMPRESSED}";
                        skip = 1;
                        break;
                    case 0x07:
                        result += "{$NORMAL_DATA_WITH_ERROR}, {$DATA_DELETED}";
                        skip = sector_size;
                        break;
                    case 0x08:
                        result += "{$NORMAL_DATA_WITH_ERROR}, {$DATA_COMPRESSED}, {$DATA_DELETED}";
                        skip = 1;
                        break;
                    default:
                        result += "{$UNKNOWN_DATA_MARKER}";
                        skip=-1;
                        break;
                }
                if (skip == 1) {
                    uint8_t data_value = 0;
                    file.read(reinterpret_cast<char*>(&data_value), 1);
                    result += " ($" + int_to_hex(data_value) + ")";
                } else
                if (skip > 0) {
                    file.seekg(skip, std::ios::cur);
                } else
                if (skip < 0) {
                    return result;
                }

                result += "\n";
            }
        }

        return result;

    }

}
