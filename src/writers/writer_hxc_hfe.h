// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A writer class for .HFE files
#pragma once


#include "writer_mfm.h"

namespace dsk_tools {

    #define HFE_BLOCK_SIZE  512
    #define HFE_TRACK_LEN   26112

    class WriterHxCHFE:public WriterMFM
    {

    protected:
        void write_hxc_hfe_header(BYTES & out);
        void write_hxc_hfe_tracks_lut(BYTES & out);
    public:
        WriterHxCHFE(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id);
        virtual std::string get_default_ext() override;
        [[nodiscard]] virtual Result write(BYTES & buffer) override;
        [[nodiscard]] virtual Result substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}
