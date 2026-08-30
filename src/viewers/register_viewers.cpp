// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Registration of all the file viewers
//
// Kept in a separate file, so that a tool showing no files does not link
// the viewers, the character maps and the BASIC detokenizers

#include "viewers/viewer.h"
#include "viewers/viewer_binary.h"
#include "viewers/agat/agat_binary.h"
#include "viewers/viewer_text.h"
#include "viewers/viewer_basic_agat.h"
#include "viewers/viewer_basic_apple.h"
#include "viewers/viewer_basic_vector.h"
#include "viewers/viewer_basic_mbasic.h"
#include "viewers/viewer_basic_iskra226.h"
#include "viewers/viewer_pics_agat.h"
#include "viewers/viewer_pics_vector.h"

namespace dsk_tools {

    void register_all_viewers() {
        static bool done = false;
        if (done) return;
        done = true;

        register_viewer<ViewerBinary>();
        register_viewer<ViewerBinaryAgat>();
        register_viewer<ViewerText>();

        register_viewer<ViewerBASIC_Agat>();
        register_viewer<ViewerBASIC_Apple>();
        register_viewer<ViewerBASIC_Vector>();
        register_viewer<ViewerBASIC_MBASIC>();
        register_viewer<ViewerBASIC_Iskra226>();

        register_viewer<ViewerPicAgat_256x256x1>();
        register_viewer<ViewerPicAgat_BMP>();
        register_viewer<ViewerPicAgat_512x256x1>();
        register_viewer<ViewerPicAgat_64x64x16>();
        register_viewer<ViewerPicAgat_128x128x16>();
        register_viewer<ViewerPicAgat_256x256x4>();
        register_viewer<ViewerPicAgat_128x256x16>();

        register_viewer<ViewerPicAgatTextT32>();
        register_viewer<ViewerPicAgatTextT64>();

        register_viewer<ViewerPicAgatFont>();
        register_viewer<ViewerPicAgatFontBFT>();

        register_viewer<ViewerPicAgat_280x192HiRes_Agat>();
        register_viewer<ViewerPicAgat_280x192HiRes_Apple>();
        register_viewer<ViewerPicAgat_140x192DblHiRes>();
        register_viewer<ViewerPicAgat_560x192DblHiResBW>();
        register_viewer<ViewerPicAgat_80x48DblLoRes>();
        register_viewer<ViewerPicAgat_40x48LoRes>();

        register_viewer<ViewerPicVectorSPR>();
    }

}
