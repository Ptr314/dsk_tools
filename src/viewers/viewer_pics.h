// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Base class for picture viewers

#pragma once

#include "disk_image.h"
#include "filesystem.h"
#include "viewer.h"

namespace dsk_tools {

    #define PREPARE_PIC_OK      0
    #define PREPARE_PIC_ERROR   1

    class ViewerPic : public Viewer {
    protected:
        int m_sx;
        int m_sy;
        int m_frame = 0;
        const BYTES * m_data;
        const dsk_tools::diskImage * m_disk_image;
        const dsk_tools::fileSystem * m_filesystem;
        virtual void start(const BYTES & data, const int frame = 0) {m_data = &data; m_frame = frame;};
        virtual uint32_t get_pixel(int x, int y) = 0;
    public:
        int get_output_type() const override {return VIEWER_OUTPUT_PICTURE;};
        virtual int get_frame_delay() const {return 0;};
        virtual int get_sx() const {return m_sx;};
        virtual int get_sy() const {return m_sy;};
        virtual int prepare_data(const BYTES & data, dsk_tools::diskImage & image, dsk_tools::fileSystem & filesystem, std::string & error_msg)
        {
            m_data = &data; m_disk_image = &image; m_filesystem = &filesystem;
            error_msg = ""; return PREPARE_PIC_OK;
        };
        virtual int suggest_option(const std::string & file_name, const BYTES & data) {return -1;};
        virtual BYTES process_picture(const BYTES & data, int & sx, int & sy, int frame = 0);
    };

}
