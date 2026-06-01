// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .DSK files

#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

#include "host_helpers.h"

#include "loader_raw.h"
#include "dsk_tools/dsk_tools.h"
#include "fs_dos33.h"
#include "utils.h"

namespace dsk_tools {
LoaderRAW::LoaderRAW(const std::string &file_name, const std::string &format_id, const std::string &type_id):
        Loader(file_name, format_id, type_id)
    {}

    Result LoaderRAW::load(BYTES &buffer, const DiskFormatParams &format)
    {
        UTF8_ifstream file(file_name, std::ios::binary);

        if (!file.good()) {
            return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Cannot open file"));
        }

        file.seekg (0, std::ios::end);
        auto fsize = file.tellg();
        file.seekg (0, std::ios::beg);

        unsigned image_size = image_size_by_type(type_id, format);
        if (image_size == 0)
            return Result::error(ErrorCode::LoadParamsMismatch, QT_TRANSLATE_NOOP("errors", "Unknown disk type"));

        // if (fsize<image_size)
        //     return Result::error(ErrorCode::LoadSizeMismatch, "File too small");

        if (fsize == image_size + 256)
            // Image with a 256-byte header?
            file.seekg (256, std::ios::beg);

        buffer.assign(image_size, 0xE5);
        file.read (reinterpret_cast<char*>(buffer.data()), image_size);

        loaded = true;

        return Result::ok();
    }

    Result LoaderRAW::load_structured(StructDisk & result, const DiskFormatParams &format)
    {
        const unsigned heads = format.heads;
        const unsigned tracks = format.tracks;
        const unsigned sectors = format.sectors;
        const unsigned sector_size = format.sector_size;

        // A raw dump has no internal structure, so the geometry must come from the image params.
        if (heads == 0 || tracks == 0 || sectors == 0 || sector_size == 0)
            return Result::error(ErrorCode::LoadParamsMismatch, QT_TRANSLATE_NOOP("errors", "Unknown disk type"));

        // Reuse load() so the optional 256-byte header and 0xE5 fill are handled identically.
        BYTES buffer;
        const auto load_result = load(buffer, format);
        if (!load_result) return load_result;

        // The image stores sectors in physical order; sector_translation maps logical -> physical.
        // Invert it once so each physical slot can be labelled with its logical (1-based) id.
        std::vector<unsigned> slot_to_logical;
        if (!format.sector_translation.empty()) {
            if (format.sector_translation.size() != sectors)
                return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Sector translation table has incorrect size"));
            slot_to_logical.assign(sectors, 0);
            for (unsigned logical = 0; logical < sectors; logical++) {
                const unsigned slot = format.sector_translation[logical];
                if (slot >= sectors)
                    return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Sector translation table has incorrect size"));
                slot_to_logical[slot] = logical;
            }
        }

        result.heads = heads;
        result.tracks.clear();

        // Cylinder-major, head-interleaved order — the layout the explorer expects.
        for (unsigned cylinder = 0; cylinder < tracks; cylinder++) {
            for (unsigned head = 0; head < heads; head++) {
                StructTrack track = {};
                track.sector_size = sector_size;
                track.cylinder = cylinder;
                track.head = head;
                track.sector_map.resize(sectors);

                // Same physical track addressing as diskImage::get_sector_data().
                unsigned track_index = cylinder * heads + head;
                if (heads == 2 && !format.sides_interleaved)
                    track_index = diskImage::transform_index(track_index, heads * tracks - 1);

                for (unsigned slot = 0; slot < sectors; slot++) {
                    StructSector sector = {};
                    sector.data.resize(sector_size);

                    const unsigned offset = (track_index * sectors + slot) * sector_size;
                    if (offset + sector_size > buffer.size())
                        return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Data exceeds buffer size"));

                    std::memcpy(sector.data.data(), buffer.data() + offset, sector_size);

                    // A sector filled entirely with 0xBD marks an unreadable/bad sector.
                    constexpr uint8_t bad_filler = 0xBD;
                    sector.is_bad = std::all_of(sector.data.begin(), sector.data.end(),
                                                [](uint8_t b){ return b == bad_filler; });

                    track.sector_map[slot] = (slot_to_logical.empty() ? slot : slot_to_logical[slot]) + 1;
                    track.sectors.push_back(sector);
                }

                result.tracks.push_back(track);
            }
        }

        return Result::ok();
    }

    std::string LoaderRAW::file_info()
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

        if (type_id == "TYPE_AGAT_140" || type_id == "TYPE_AGAT_840") {
            BYTES buffer(fsize);
            if (load(buffer)) {
                uint32_t vtoc_pos;
                if (type_id == "TYPE_AGAT_140") vtoc_pos=17*16*256;
                else
                if (type_id == "TYPE_AGAT_840") vtoc_pos=17*21*256;

                Agat_VTOC * VTOC = reinterpret_cast<Agat_VTOC *>(buffer.data() + vtoc_pos);
                result += agat_vtoc_info(*VTOC);
            } else {
                result += "{$ERROR_LOADING}\n";
            }
        }

        return result;

    }

}
