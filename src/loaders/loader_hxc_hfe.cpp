// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .HFE files

#include <fstream>
#include <iostream>

#include "host_helpers.h"

#include "dsk_tools/dsk_tools.h"
#include "definitions.h"
#include "utils.h"
#include "loader_hxc_hfe.h"

namespace dsk_tools {

    namespace {

        bool is_agat_type(const std::string & type_id)
        {
            return type_id == "TYPE_AGAT_840" || type_id == "TYPE_AGAT_880";
        }

        Result read_file(const std::string & file_name, BYTES & in)
        {
            UTF8_ifstream file(file_name, std::ios::binary);
            if (!file.good()) return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Cannot open file"));
            file.seekg (0, std::ios::end);
            const auto fsize = file.tellg();
            file.seekg (0, std::ios::beg);
            in.resize(static_cast<size_t>(fsize));
            file.read (reinterpret_cast<char*>(in.data()), fsize);
            return Result::ok();
        }

    }

    LoaderHXC_HFE::LoaderHXC_HFE(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    Result LoaderHXC_HFE::read_image(HfeImage & img)
    {
        BYTES in;
        const Result res = read_file(file_name, in);
        if (!res) return res;
        return hfe_read(in, img);
    }

    // Agat 840 and 880 Kb disks are encoded identically apart from the length of a
    // data field, so the format is told apart by the checksum the disk agrees with
    Result LoaderHXC_HFE::probe_sector_size(int & sector_size)
    {
        sector_size = 256;

        HfeImage img;
        const Result res = read_image(img);
        if (!res) return res;

        BYTES track_data;
        decode_agat_mfm_data(track_data, img.cells[0]);
        sector_size = detect_agat_sector_size(track_data);

        return Result::ok();
    }

    Result LoaderHXC_HFE::load_agat(const HfeImage & img, BYTES & buffer)
    {
        if (img.sides != 2 || img.tracks != 80)
            return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Invalid HFE parameters"));

        const bool nippel = (type_id == "TYPE_AGAT_880");
        const int sectors_per_track = nippel ? 11 : 21;
        const int s_size = nippel ? 512 : 256;
        buffer.resize(2*80*sectors_per_track*s_size);

        bool errors = false;
        for (int track=0; track < img.tracks; track++) {
            for (int s=0; s < img.sides; s++) {
                BYTES track_data;
                decode_agat_mfm_data(track_data, img.cells[track * img.sides + s]);

                BYTES raw_data;
                const Result res = decode_agat_840_track(raw_data, track_data, sectors_per_track, s_size);
                if (res)
                    std::copy(raw_data.begin(), raw_data.end(),
                              buffer.begin() + (((track << 1) + s) * sectors_per_track) * s_size);
                else
                    errors = true;
            }
        }
        loaded = true;
        if (errors) return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Failed to decode track data"));
        return Result::ok();
    }

    Result LoaderHXC_HFE::load(BYTES &buffer, const DiskFormatParams &format)
    {
        HfeImage img;
        const Result res = read_image(img);
        if (!res) return res;

        if (is_agat_type(type_id)) return load_agat(img, buffer);

        if (format.layout == TrackLayout::None || format.heads == 0 || format.tracks == 0
            || format.sectors == 0 || format.sector_size == 0)
            return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Unsupported disk type"));

        m_bad_sectors.clear();
        hfe_to_flat(img, format, buffer, &m_bad_sectors);
        loaded = true;
        return Result::ok();
    }

    // Every track as it is on the disk: the sectors in the order they pass the
    // head, with the numbers their ID fields carry
    Result LoaderHXC_HFE::load_structured(StructDisk & result, const DiskFormatParams &format)
    {
        if (format.layout == TrackLayout::None)
            return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Not implemented"));

        HfeImage img;
        const Result res = read_image(img);
        if (!res) return res;

        result.heads = static_cast<unsigned>(img.sides);
        result.tracks.clear();
        for (int t = 0; t < img.tracks; t++)
            for (int h = 0; h < img.sides; h++) {
                const BYTES & cells = img.cells[static_cast<size_t>(t) * img.sides + h];
                StructTrack track = {};
                track.cylinder = static_cast<uint8_t>(t);
                track.head = static_cast<uint8_t>(h);
                track.sector_size = format.sector_size;

                if (format.layout == TrackLayout::DvkMx) {
                    // No IDs on an MX track: the sectors simply follow the sync word
                    BYTES flat(static_cast<size_t>(format.sectors) * format.sector_size, 0);
                    TrackStatus status;
                    if (track_from_cells(format, cells, flat.data(), status))
                        for (unsigned s = 0; s < format.sectors; s++) {
                            StructSector sector = {};
                            sector.is_bad = (status.bad_crc & (1u << s)) != 0;
                            sector.data.assign(flat.begin() + s * format.sector_size, flat.begin() + (s + 1) * format.sector_size);
                            track.sectors.push_back(sector);
                            track.sector_map.push_back(static_cast<uint8_t>(s + 1));
                        }
                } else {
                    BYTES data;
                    IbmTrackFields fields;
                    track_decode(format, cells, data, fields);
                    for (const IbmSectorField & f : fields.sectors) {
                        if (!f.id_crc_ok) continue;
                        StructSector sector = {};
                        const size_t size = static_cast<size_t>(f.size());
                        if (f.data_pos != IBM_NO_DATA) {
                            sector.data.assign(data.begin() + f.data_pos + 1, data.begin() + f.data_pos + 1 + size);
                            sector.is_bad = !f.data_crc_ok;
                        } else {
                            sector.data.assign(size, 0);
                            sector.is_bad = true;
                        }
                        if (track.sectors.empty()) track.sector_size = static_cast<unsigned>(size);
                        track.sectors.push_back(sector);
                        track.sector_map.push_back(f.sector);
                        track.cylinder_map.push_back(f.track);
                        track.head_map.push_back(f.side);
                    }
                }
                result.tracks.push_back(track);
            }
        return Result::ok();
    }

    std::string LoaderHXC_HFE::agat_track_info(const BYTES & track_data, bool & errors)
    {
        std::string result;
        const int track_len = static_cast<int>(track_data.size());
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
            if (!index_found) continue;
            if (in_p + 4 > track_len) break;
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
            if (!data_found) continue;
            result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p-2)) + " {$DATA_MARK} ($6A $95)\n";
            result += "    $" + dsk_tools::int_to_hex(static_cast<uint16_t>(in_p)) + " {$DATA_FIELD} (256)\n";

            bool error = false;
            uint16_t crc = 0;
            if (in_p + 16 <= track_len)
                result += "               " + dsk_tools::toHexList(track_data.data() + in_p, 16) + " ...\n";

            for (int i=0; i<256; i++) {
                if (in_p >= track_len) {error = true; break;};
                uint8_t  d = track_data.at(in_p++);
                if (crc > 0xFF) crc = (crc + 1) & 0xFF;
                crc += d;
            }
            crc &= 0xFF;
            if (!error && in_p + 2 <= track_len) {
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
        return result;
    }

    // The fields of an IBM track: the index mark, and for every sector its ID
    // (C/H/R/N), the data mark and both CRCs
    std::string LoaderHXC_HFE::ibm_track_info(const DiskFormatParams & format, const BYTES & cells, bool & errors)
    {
        std::string result;
        BYTES data;
        IbmTrackFields fields;
        track_decode(format, cells, data, fields);
        for (size_t p : fields.index_marks)
            result += "    $" + int_to_hex(static_cast<uint16_t>(p)) + " {$INDEX_MARK}\n";
        for (const IbmSectorField & f : fields.sectors) {
            result += "    $" + int_to_hex(static_cast<uint16_t>(f.id_pos)) + " {$SECTOR_INDEX}:"
                      + " {$TRACK_SHORT}=" + std::to_string(f.track)
                      + ", {$SIDE_SHORT}=" + std::to_string(f.side)
                      + ", {$LOGICAL_SECTOR}=" + std::to_string(f.sector)
                      + ", {$SIZE_CODE_SHORT}=" + std::to_string(f.size_code) + " (" + std::to_string(f.size()) + ")"
                      + ", " + (f.id_crc_ok ? "{$INDEX_CRC_OK}" : "{$INDEX_CRC_ERROR}") + "\n";
            if (!f.id_crc_ok) {
                errors = true;
                continue;
            }
            if (f.data_pos == IBM_NO_DATA) {
                errors = true;
                result += "    {$NO_DATA_FIELD}\n";
                continue;
            }
            result += "    $" + int_to_hex(static_cast<uint16_t>(f.data_pos)) + " {$DATA_MARK} ($"
                      + int_to_hex(data[f.data_pos]) + ", " + (f.deleted ? "{$DATA_DELETED}" : "{$NORMAL_DATA}") + ")"
                      + ", " + (f.data_crc_ok ? "{$SECTOR_CRC_OK}" : "{$SECTOR_CRC_ERROR}") + "\n";
            result += "               " + toHexList(data.data() + f.data_pos + 1, 16) + " ...\n";
            if (!f.data_crc_ok) errors = true;
        }
        if (fields.sectors.empty()) {
            result += "    {$SECTOR_UNAVAILABLE}\n";
            errors = true;
        }
        return result;
    }

    std::string LoaderHXC_HFE::file_info()
    {
        std::string result = "";

        BYTES in;
        if (!read_file(file_name, in)) {
            result += "{$ERROR_OPENING}:\n";
            return result;
        }

        size_t pos = file_name.find_last_of("/\\");
        std::string file_short = (pos == std::string::npos) ? file_name : file_name.substr(pos + 1);
        result += "{$FILE_NAME}: " + file_short + "\n";
        result += "{$SIZE}: " + std::to_string(in.size()) + " {$BYTES}\n";

        if (in.size() < sizeof(HXC_HFE_HEADER)) {
            result += "\n{$NO_SIGNATURE}\n";
            return result;
        }
        const HXC_HFE_HEADER * hdr = reinterpret_cast<const HXC_HFE_HEADER*>(in.data());
        const std::string signature(reinterpret_cast<const char*>(&hdr->HEADERSIGNATURE), sizeof(hdr->HEADERSIGNATURE));
        if (signature != "HXCPICFE") {
            result += "\n{$NO_SIGNATURE}\n";
            return result;
        }

        result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(0)) + " {$HEADER}\n";
        result += "    {$SIGNATURE}: " + signature + "\n";
        result += "    {$FORMAT_REVISION}: " + std::to_string(hdr->formatrevision) + "\n";
        result += "    {$TRACKS}: " + std::to_string(hdr->number_of_track) + "\n";
        result += "    {$SIDES}: " + std::to_string(hdr->number_of_side) + "\n";
        result += "    {$TRACKLIST_OFFSET}: " + std::to_string(hdr->track_list_offset) + "*512 ($" + dsk_tools::int_to_hex(hdr->track_list_offset*HXC_HFE_BLOCK_SIZE, false) + ")\n";

        const size_t tracklist_offset = static_cast<size_t>(hdr->track_list_offset) * HXC_HFE_BLOCK_SIZE;
        result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(tracklist_offset)) + " {$TRACKLIST_OFFSET}\n";

        std::vector<HXC_HFE_TRACK> ti(hdr->number_of_track);
        for (int track=0; track < hdr->number_of_track; track++) {
            const size_t at = tracklist_offset + track * sizeof(HXC_HFE_TRACK);
            if (at + sizeof(HXC_HFE_TRACK) > in.size()) break;
            memcpy(&ti[track], in.data() + at, sizeof(HXC_HFE_TRACK));
            result += "    " + std::to_string(track) + ":"
                      + " {$TRACK_OFFSET}: $" + dsk_tools::int_to_hex(ti[track].offset*HXC_HFE_BLOCK_SIZE, false)
                      + ", {$TRACK_SIZE}: $" + dsk_tools::int_to_hex(ti[track].track_len, false)
                      + "\n";
        }
        result += "\n";

        HfeImage img;
        if (!hfe_read(in, img)) {
            result += "{$ERROR_PARSING}\n";
            return result;
        }

        // The Agat by its type; any other disk by the layout its tracks show
        DiskFormatParams format;
        const bool agat = is_agat_type(type_id) || !hfe_probe_format(img, format);
        if (!agat) result += "{$TRACK_LAYOUT}: " + track_layout_name(format.layout) + "\n\n";
        if (!agat && format.layout == TrackLayout::DvkMx) return result;

        bool errors = false;
        for (int track=0; track < img.tracks; track++)
            for (int s=0; s < img.sides; s++) {
                const BYTES & cells = img.cells[static_cast<size_t>(track) * img.sides + s];
                result += "$" + dsk_tools::int_to_hex(static_cast<uint32_t>(ti[track].offset*HXC_HFE_BLOCK_SIZE))
                          + ": {$TRACK} " + std::to_string(track) + " {$SIDE} " + std::to_string(s) + "\n";
                if (agat) {
                    BYTES track_data;
                    decode_agat_mfm_data(track_data, cells);
                    result += agat_track_info(track_data, errors);
                } else
                    result += ibm_track_info(format, cells, errors);
            }
        result += errors ? "{$ERROR_PARSING}\n" : "{$PARSING_FINISHED}\n";
        return result;
    }

}
