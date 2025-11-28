// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a host filesystem

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cerrno>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_host.h"

#ifdef _WIN32
    #include <windows.h>

    // Windows-specific wrappers for UTF-8 file I/O

    // Wrapper for std::ifstream that handles UTF-8 paths on Windows
    class UTF8_ifstream : public std::ifstream {
    public:
        explicit UTF8_ifstream(const std::string& filename,
                              std::ios_base::openmode mode = std::ios_base::in)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            this->open(wpath.c_str(), mode);
        }
    };

    // Wrapper for std::ofstream that handles UTF-8 paths on Windows
    class UTF8_ofstream : public std::ofstream {
    public:
        explicit UTF8_ofstream(const std::string& filename,
                              std::ios_base::openmode mode = std::ios_base::out)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            this->open(wpath.c_str(), mode);
        }
    };

    // Helper function for file removal with UTF-8 path
    inline int utf8_remove(const std::string& path) {
        std::wstring wpath = dsk_tools::utf8_to_wide(path);
        return _wremove(wpath.c_str());
    }

    // Helper function for directory creation with UTF-8 path
    inline int utf8_mkdir(const std::string& path) {
        std::wstring wpath = dsk_tools::utf8_to_wide(path);
        return _wmkdir(wpath.c_str());
    }

#else
    // On non-Windows platforms, use standard classes directly
    using UTF8_ifstream = std::ifstream;
    using UTF8_ofstream = std::ofstream;

    inline int utf8_remove(const std::string& path) {
        return std::remove(path.c_str());
    }

    inline int utf8_mkdir(const std::string& path) {
        return ::mkdir(path.c_str(), 0755);
    }
#endif

namespace dsk_tools {

    fsHost::fsHost(diskImage * image):
        fileSystem(nullptr)
    {}

    FSCaps fsHost::getCaps()
    {
        return FSCaps::Delete | FSCaps::Add | FSCaps::Dirs | FSCaps::Rename| FSCaps::MkDir;
    }

    int fsHost::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;
        is_open = true;
        return FDD_OPEN_OK;
    }

    std::vector<std::string> fsHost::get_save_file_formats()
    {
        return {};
    }

    Result fsHost::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (uf.fs != getFS()) return Result::error(ErrorCode::FileIncorrectFS);

        std::string path = bytesToString(uf.metadata);
        std::cout << "Host: get_file " << path << std::endl;

        // Open file in binary mode
        UTF8_ifstream file(path, std::ios::binary);
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

        // std::str file_name = uf.name;
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
        UTF8_ifstream testFile(fullPath);
        if (testFile.good()) {
            testFile.close();
            if (!force_replace) {
                return Result::error(ErrorCode::FileAlreadyExists);
            }
        }

        // Open file in binary write mode
        UTF8_ofstream file(fullPath, std::ios::binary);
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
        UTF8_ifstream testFile(path);
        if (!testFile.good()) {
            return Result::error(ErrorCode::FileNotFound);
        }
        testFile.close();

        // Delete the file
        if (utf8_remove(path) != 0) {
            return Result::error(ErrorCode::FileDeleteError);
        }

        return Result::ok();
    }

    Result fsHost::dir(std::vector<dsk_tools::UniversalFile> & files, bool show_deleted)
    {
        files.clear();
        return Result::error(ErrorCode::NotImplementedYet);
    }

    Result fsHost::mkdir(const std::string & dir_name)
    {
        // std::cout << "Host: mkdir " << m_path << " + " << dir_name << std::endl;

        std::string fullPath;
        if (m_path.empty()) {
            fullPath = dir_name;
        } else {
            fullPath = m_path;
            char lastChar = m_path[m_path.length() - 1];
            if (lastChar != '/' && lastChar != '\\') {
                fullPath += '/';
            }
            fullPath += dir_name;
        }

        // Platform-specific directory creation with UTF-8 support
        int result = utf8_mkdir(fullPath);

        if (result != 0) {
            // Check if directory already exists
            if (errno == EEXIST) {
                return Result::error(ErrorCode::DirAlreadyExists);
            }
            return Result::error(ErrorCode::DirError);
        }

        return Result::ok();
    }

}
