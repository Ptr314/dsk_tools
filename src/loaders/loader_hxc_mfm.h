// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A loader class for .MFM files

#ifndef LOADER_HXC_MFM_H
#define LOADER_HXC_MFM_H

#include "loader_mfm.h"

namespace dsk_tools {

    class LoaderHXC_MFM:public LoaderMFM
    {
    public:
        LoaderHXC_MFM(const std::string & file_name, const std::string & format_id, const std::string & type_id);
    protected:
        virtual void prepare_tracks_list(BYTES & in) override;
        virtual std::string get_header_info(BYTES & in) override;

    };

}

#endif // LOADER_HXC_MFM_H
