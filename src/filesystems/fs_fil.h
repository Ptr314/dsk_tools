// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - filesystem part
#pragma once



#include "filesystem.h"

namespace dsk_tools {

    class fsFIL: public fileSystem
    {

    public:
        explicit fsFIL(diskImage * image);
        int open() override;
        FSCaps getCaps() override;
        FS getFS() const override {return FS::DOS33;};
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        std::string file_info(const UniversalFile & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        Result rename_file(const UniversalFile & fd, const std::string & new_name) override;
        std::vector<ParameterDescription> file_get_metadata(const UniversalFile & fd) override;
        Result file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata) override;
    };
}
