// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .HFE files

#include <cstring>
#include <iostream>
#include <fstream>

#include "dsk_tools/core.h"
#include "writer_hxc_hfe.h"


namespace dsk_tools {

WriterHxCHFE::WriterHxCHFE(const std::string & format_id, diskImage * image_to_save, const uint8_t volume_id):
        WriterMFM(format_id, image_to_save, volume_id)
    {}

    std::string WriterHxCHFE::get_default_ext()
    {
        return "hfe";
    }

    void WriterHxCHFE::write_hxc_hfe_header(BYTES & out)
    {
        HXC_HFE_HEADER header;
        memset(&header, 0xFF, sizeof(header));

        std::strncpy(reinterpret_cast<char *>(&header.HEADERSIGNATURE[0]), "HXCPICFE", 8);
        header.formatrevision = 0;
        header.number_of_track = image->get_tracks();
        header.number_of_side = image->get_heads();
        header.track_encoding = image->get_track_encoding();
        header.bitRate = image->get_bitrate();
        header.floppyRPM = 0; //image->get_rpm();
        header.floppyinterfacemode = image->get_floppyinterfacemode();
        header.write_protected = 0x00;
        header.track_list_offset = 512 / HFE_BLOCK_SIZE;
        header.write_allowed = 0x00;

        // v1.1 extended fields
        header.single_step = 0xFF;
        header.track0s0_altencoding = 0xFF;
        header.track0s0_encoding = 0xFF;
        header.track0s1_altencoding = 0xFF;
        header.track0s1_encoding = 0xFF;

        uint8_t * ptr = reinterpret_cast<uint8_t*>(&header);
        out.insert(out.end(), ptr, ptr + sizeof(header));
        out.insert(out.end(), 512 - sizeof(header), 0xFF);
    }

    void WriterHxCHFE::write_hxc_hfe_tracks_lut(std::vector<uint8_t> & out)
    {
        HXC_HFE_TRACK track;
        uint8_t * ptr = reinterpret_cast<uint8_t*>(&track);
        int offset = 2;
        track.track_len = 12928 * 2; //TODO: calculate this value
        for (int i=0; i < image->get_tracks(); i++) {
            track.offset = offset;
            offset += HFE_TRACK_LEN / HFE_BLOCK_SIZE;

            out.insert(out.end(), ptr, ptr + sizeof(track));
        };

        out.insert(out.end(), HFE_BLOCK_SIZE - sizeof(track) * image->get_tracks(), 0xFF);
    }

    // The RT-11 disks of the DVK, the БК and the УК-НЦ: every track is laid
    // out as its controller formats it and turned into cells by the core
    Result WriterHxCHFE::write_rt11(BYTES &buffer, int kind)
    {
        HfeImage img;
        rt11_hfe_params(kind, img);
        img.tracks = image->get_tracks();
        img.sides = image->get_heads();
        img.cells.resize((size_t)img.tracks * img.sides);
        const int sectors = image->get_sectors();
        const int sector_size = image->get_sector_size();
        BYTES flat((size_t)sectors * sector_size);
        for (int t = 0; t < img.tracks; t++)
            for (int h = 0; h < img.sides; h++) {
                for (int sec = 0; sec < sectors; sec++) {
                    const uint8_t * data = image->get_sector_data(h, t, sec);
                    if (data != nullptr) memcpy(flat.data() + (size_t)sec * sector_size, data, sector_size);
                    else memset(flat.data() + (size_t)sec * sector_size, 0, sector_size);
                }
                rt11_track_cells(kind, t, h, sectors, sector_size, flat.data(), img.cells[(size_t)t * img.sides + h]);
            }
        hfe_write(img, buffer);
        return Result::ok();
    }

    Result WriterHxCHFE::write(BYTES &buffer)
    {
        std::string type_id = image->get_type_id();
        const int kind = rt11_track_kind(type_id);
        if (kind != 0) return write_rt11(buffer, kind);
        if (type_id != "TYPE_AGAT_840" && type_id != "TYPE_AGAT_880")
            return Result::error(ErrorCode::WriteUnsupported, QT_TRANSLATE_NOOP("errors", "Format not supported for HFE format"));

        buffer.clear();
        buffer.reserve(buffer.size() + 512);

        write_hxc_hfe_header(buffer);
        write_hxc_hfe_tracks_lut(buffer);

        std::vector<BYTES> track_buffer(image->get_heads());

        for (uint8_t track = 0; track < image->get_tracks(); track++)
        {
            for (uint8_t head = 0; head < image->get_heads(); head++)
            {
                track_buffer[head].clear();
                write_agat840_track(track_buffer[head], head, track);
            }
            int blocks = 13056*2 / HFE_BLOCK_SIZE; // TODO: calculate
            for (int block=0; block < blocks; block++)
            {
                for (uint8_t head=0; head < 2; head ++)
                {
                    uint8_t * ptr = track_buffer[head].data() + block*256;
                    buffer.insert(buffer.end(), ptr, ptr+256);
                }
            }
        }
        return Result::ok();
    }

    Result WriterHxCHFE::substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks)
    {
        return Result::error(ErrorCode::WriteUnsupported, QT_TRANSLATE_NOOP("errors", "Track substitution not supported for HFE format"));
    }

}
