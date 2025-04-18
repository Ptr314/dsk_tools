// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Base class for picture viewers

#pragma once

#include "viewer.h"

namespace dsk_tools {

    typedef std::vector<std::pair<int, std::string>> PicOptions;

    class ViewerPic : public Viewer {
    protected:
        int m_sx;
        int m_sy;
        int m_opt = 0;
        const BYTES * m_data;
        virtual void start(const BYTES & data, int opt) {m_data = &data; m_opt = opt;};
        virtual uint32_t get_pixel(int x, int y) = 0;
    public:
        int get_output_type() const override {return VIEWER_OUTPUT_PICTURE;};
        virtual int get_sx() const {return m_sx;};
        virtual int get_sy() const {return m_sy;};
        virtual PicOptions get_options() {return {};};
        virtual BYTES process_picture(const BYTES & data, int & sx, int & sy, const int opt);
    };

}
