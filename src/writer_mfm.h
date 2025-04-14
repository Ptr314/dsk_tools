// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A top level abstract writer class for some psysical formats

#ifndef WRITER_MFM_H
#define WRITER_MFM_H

#include "writer.h"

namespace dsk_tools {

    #define AGAT_140_GAP0    48
    #define AGAT_140_GAP1    6
    #define AGAT_140_GAP2    27
    #define AGAT_140_GAP3    (track_length - (AGAT_140_GAP0 + 16 * (3 + 8 + 3 + AGAT_140_GAP1 + 3 + 343 + 3 + AGAT_140_GAP2)))

    class WriterMFM:public Writer
    {
    protected:
        uint8_t m_volume_id;

        void write_gcr62_track(BYTES &out, uint8_t track, int track_length);
        void write_gcr62_nic_track(BYTES &out, uint8_t track);
        void write_agat840_track(BYTES &out, uint8_t head, uint8_t track);
    public:
        WriterMFM(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id);

    };

}

#endif // WRITER_MFM_H
