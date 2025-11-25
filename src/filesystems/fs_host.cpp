// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a host filesystem

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_host.h"

namespace dsk_tools {

    fsHost::fsHost(diskImage * image):
        fileSystem(nullptr)
    {}

    FSCaps fsHost::getCaps()
    {
        return FSCaps::Delete | FSCaps::Add | FSCaps::Dirs | FSCaps::Rename;
    }

    int fsHost::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        is_open = true;

        return FDD_OPEN_OK;
    }

    int fsHost::dir(std::vector<dsk_tools::fileData> * files, bool show_deleted)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;
        files->clear();
        return FDD_OP_OK;
    }

    BYTES fsHost::get_file(const fileData & fd)
    {
        BYTES data;
        return data;
    }

    std::string fsHost::file_info(const fileData & fd)
    {

        std::string result = "";
        return result;
    }

    std::vector<std::string> fsHost::get_save_file_formats()
    {
        return {};
    }

    int fsHost::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
    {
        return FDD_WRITE_UNSUPPORTED;
    }

    std::string fsHost::information()
    {
        std::string result;
        return result;
    }


    int fsHost::file_rename(const fileData & fd, const std::string & new_name)
    {
        return FILE_RENAME_OK;
    }

    std::vector<ParameterDescription> fsHost::file_get_metadata(const fileData &fd)
    {
        std::vector<ParameterDescription> params;
        return params;
    }

    int fsHost::file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata)
    {
        return FILE_METADATA_OK;
    }

    bool fsHost::file_find(const std::string & file_name, fileData & fd)
    {
        return false;
    }

    Result fsHost::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (uf.fs != getFS()) return Result::error(ErrorCode::FileIncorrectFS);

        std::string path = bytesToString(uf.metadata);
        std::cout << "Host: get_file " << path << std::endl;

        // Open file in binary mode
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return Result::error(ErrorCode::FileNotFound);
        }

        // Determine file size
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Resize data vector and read file contents
        data.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            file.close();
            return Result::error(ErrorCode::ReadError);
        }

        file.close();
        return Result::ok();
    }

    Result fsHost::put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace)
    {
        std::cout << "Host: put_file " << m_path << " + " << uf.name << std::endl;

        // Construct full file path
        std::string fullPath;
        if (m_path.empty()) {
            fullPath = uf.name;
        } else {
            fullPath = m_path;
            char lastChar = m_path[m_path.length() - 1];
            if (lastChar != '/' && lastChar != '\\') {
                fullPath += '/';  // Use forward slash (works on all platforms)
            }
            fullPath += uf.name;
        }

        if (format == "FILE_FIL") {
            fullPath += ".fil";
        }

        // Check if file already exists
        std::ifstream testFile(fullPath);
        if (testFile.good()) {
            testFile.close();
            if (!force_replace) {
                return Result::error(ErrorCode::FileAlreadyExists);
            }
        }

        // Open file in binary write mode
        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            return Result::error(ErrorCode::WriteError);
        }

        // Write data to file
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        if (!file.good()) {
            file.close();
            return Result::error(ErrorCode::WriteError);
        }

        file.close();
        return Result::ok();
    }

    Result fsHost::delete_file(const UniversalFile & uf)
    {
        // Validate filesystem type
        if (uf.fs != getFS()) return Result::error(ErrorCode::FileIncorrectFS);

        // Extract path from metadata
        std::string path = bytesToString(uf.metadata);
        std::cout << "Host: delete_file " << path << std::endl;

        // Check if file exists
        std::ifstream testFile(path);
        if (!testFile.good()) {
            return Result::error(ErrorCode::FileNotFound);
        }
        testFile.close();

        // Delete the file
        if (std::remove(path.c_str()) != 0) {
            return Result::error(ErrorCode::FileDeleteError);
        }

        return Result::ok();
    }

    Result fsHost::dir(std::vector<dsk_tools::UniversalFile> & files, bool show_deleted)
    {
        files.clear();
        return Result::error(ErrorCode::NotImplementedYet);
    }


}
