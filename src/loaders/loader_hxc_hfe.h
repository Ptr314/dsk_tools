// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .HFE files
#pragma once


#include "loader.h"

namespace dsk_tools {

    struct HfeImage;

    // Agat 840/880 Kb disks are decoded by their own track format; every other
    // type by the TrackLayout of its diskdef (IBM MFM, IBM FM, DVK MX)
    class LoaderHXC_HFE:public Loader
    {
    protected:
        Result read_image(HfeImage & img);
        Result load_agat(const HfeImage & img, BYTES & buffer);
        std::string agat_track_info(const BYTES & track_data, bool & errors);
        std::string ibm_track_info(const DiskFormatParams & format, const BYTES & cells, bool & errors);
    public:
        LoaderHXC_HFE(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        Result load(BYTES & buffer, const DiskFormatParams &format = DiskFormatParams()) override;
        Result load_structured(StructDisk & result, const DiskFormatParams &format = DiskFormatParams()) override;
        // Tells an 840 Kb disk from an 880 Kb one by the data of its first track
        Result probe_sector_size(int & sector_size);
        std::string file_info() override;
    };

}
