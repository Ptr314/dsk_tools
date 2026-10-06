// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .HFE files
#pragma once


#include "writer_mfm.h"

namespace dsk_tools {

    struct HfeImage;

    #define HFE_BLOCK_SIZE  512
    #define HFE_TRACK_LEN   26112

    // Agat 840/880 Kb disks by their own track encoder, every other type by the
    // TrackLayout of its diskdef
    class WriterHxCHFE:public WriterMFM
    {

    protected:
        Result write_agat(HfeImage & img);
        Result write_tracks(HfeImage & img);
    public:
        WriterHxCHFE(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id);
        std::string get_default_ext() override;
        Result write(BYTES & buffer) override;
        Result substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}
