// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a host filesystem

#pragma once

#include "filesystem.h"

namespace dsk_tools {

    class fsHost: public fileSystem
    {
    private:
        std::string m_path;

    public:
        explicit fsHost(diskImage * image);
        int open() override;
        FSCaps getCaps() override;
        FS getFS() const override {return FS::Host;};
        Result dir(std::vector<UniversalFile> & files, bool show_deleted) override;
        std::vector<std::string> get_save_file_formats() override;
        Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const override;
        Result put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace) override;
        Result delete_file(const UniversalFile & uf) override;
        void cd(const std::string & path) override {m_path = path;};
        Result mkdir(const std::string & dir_name) override;


    };
}

