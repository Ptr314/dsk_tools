// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - filesystem part

#ifndef FS_FIL_H
#define FS_FIL_H


#include "filesystem.h"

namespace dsk_tools {

    class fsFIL: public fileSystem
    {

    public:
        explicit fsFIL(diskImage * image);
        int open() override;
        FSCaps getCaps() override;
        FS getFS() const override {return FS::DOS33;};
        int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) override;
        BYTES get_file(const fileData & fd) override;
        Result get_file(const UniversalFile & uf, BYTES & data) const override;
        Result put_file(const UniversalFile & uf, const BYTES & data, bool force_replace = false) override;
        std::string file_info(const fileData & fd) override;
        std::vector<std::string> get_save_file_formats() override;
        int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
        std::string information() override;
        int file_rename(const fileData & fd, const std::string & new_name) override;
        std::vector<ParameterDescription> file_get_metadata(const fileData & fd) override;
        int file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata) override;
        bool file_find(const std::string & file_name, fileData &fd) override;
    };
}

#endif // FS_FIL_H
