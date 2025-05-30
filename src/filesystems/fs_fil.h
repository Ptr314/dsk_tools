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
        fsFIL(diskImage * image);
        virtual int open() override;
        virtual int get_capabilities() override;
        virtual int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) override;
        virtual BYTES get_file(const fileData & fd) override;
        virtual std::string file_info(const fileData & fd) override;
        virtual std::vector<std::string> get_save_file_formats() override;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
        virtual std::string information() override;
        virtual int file_rename(const fileData & fd, const std::string & new_name) override;
        virtual std::vector<ParameterDescription> file_get_metadata(const fileData & fd) override;
        virtual int file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata) override;
        bool file_find(const std::string & file_name, fileData &fd) override;
    };
}

#endif // FS_FIL_H
