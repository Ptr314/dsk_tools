// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .DSK files

#include "writer_raw.h"

namespace dsk_tools {

    WriterRAW::WriterRAW(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    std::string WriterRAW::get_default_ext()
    {
        return "dsk";
    }


    Result WriterRAW::write(BYTES &buffer)
    {
        buffer = *image->get_buffer();
        return Result::ok();
    }

    Result WriterRAW::substitute_tracks(BYTES & buffer, BYTES &tmplt, const int numtracks)
    {
        if (buffer.size() != tmplt.size())
            return Result::error(ErrorCode::WriteIncorrectTemplate, "Template file size mismatch");
        if (buffer.size() != image->get_size())
            return Result::error(ErrorCode::WriteIncorrectSource, "Source file size mismatch");
        int block_size = image->get_sectors() * image->get_sector_size() * image->get_heads();

        BYTES out;
        out.insert(out.end(), tmplt.begin(), tmplt.begin() + block_size);
        out.insert(out.end(), buffer.begin() + block_size, buffer.end());

        buffer = out;
        return Result::ok();
    }
}
