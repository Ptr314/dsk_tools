// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .HFE files

#include <cstring>

#include "dsk_tools/core.h"
#include "writer_hxc_hfe.h"


namespace dsk_tools {

    // An Agat track as the HFE keeps it: the cells up to this length, the rest
    // of the last block is padding
    #define HFE_AGAT_TRACK_CELLS    12928

    WriterHxCHFE::WriterHxCHFE(const std::string & format_id, diskImage * image_to_save, const uint8_t volume_id):
        WriterMFM(format_id, image_to_save, volume_id)
    {}

    std::string WriterHxCHFE::get_default_ext()
    {
        return "hfe";
    }

    // The Agat 840/880 Kb disks: tracks of the Agat encoder, the header as the
    // earlier versions wrote it (no rpm, write protected)
    Result WriterHxCHFE::write_agat(HfeImage & img)
    {
        img.tracks = image->get_tracks();
        img.sides = image->get_heads();
        img.bitrate = static_cast<uint16_t>(image->get_bitrate());
        img.rpm = 0;
        img.encoding = static_cast<uint8_t>(image->get_track_encoding());
        img.interface_mode = static_cast<uint8_t>(image->get_floppyinterfacemode());
        img.write_allowed = false;
        img.cells.assign(static_cast<size_t>(img.tracks) * img.sides, BYTES());
        for (int track = 0; track < img.tracks; track++)
            for (int head = 0; head < img.sides; head++) {
                BYTES & cells = img.cells[static_cast<size_t>(track) * img.sides + head];
                write_agat840_track(cells, head, track);
                cells.resize(HFE_AGAT_TRACK_CELLS);
            }
        return Result::ok();
    }

    // Any other disk by the TrackLayout of its diskdef: every track laid out as
    // its controller formats it
    Result WriterHxCHFE::write_tracks(HfeImage & img)
    {
        const DiskFormatParams & format = image->get_format();
        track_hfe_params(format, img);
        const int sectors = image->get_sectors();
        const int sector_size = image->get_sector_size();
        BYTES flat(static_cast<size_t>(sectors) * sector_size);
        for (int t = 0; t < img.tracks; t++)
            for (int h = 0; h < img.sides; h++) {
                // Sector n of the image is the sector numbered sector_base + n on
                // the track, whatever order the filesystem reads them in
                const uint8_t * data = image->get_buffer()->data();
                const size_t at = (static_cast<size_t>(t) * img.sides + h) * flat.size();
                if (at + flat.size() <= image->get_buffer()->size())
                    memcpy(flat.data(), data + at, flat.size());
                else
                    std::fill(flat.begin(), flat.end(), 0);
                if (!track_to_cells(format, t, h, flat.data(), img.cells[static_cast<size_t>(t) * img.sides + h]))
                    return Result::error(ErrorCode::WriteUnsupported, QT_TRANSLATE_NOOP("errors", "Format not supported for HFE format"));
            }
        return Result::ok();
    }

    Result WriterHxCHFE::write(BYTES &buffer)
    {
        const std::string type_id = image->get_type_id();
        HfeImage img;
        Result res;
        if (type_id == "TYPE_AGAT_840" || type_id == "TYPE_AGAT_880")
            res = write_agat(img);
        else
        if (image->get_format().layout != TrackLayout::None)
            res = write_tracks(img);
        else
            return Result::error(ErrorCode::WriteUnsupported, QT_TRANSLATE_NOOP("errors", "Format not supported for HFE format"));
        if (!res) return res;
        hfe_write(img, buffer);
        return Result::ok();
    }

    Result WriterHxCHFE::substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks)
    {
        return Result::error(ErrorCode::WriteUnsupported, QT_TRANSLATE_NOOP("errors", "Track substitution not supported for HFE format"));
    }

}
