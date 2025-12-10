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
    #include <windows.h>
    #include <io.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_host.h"
#include "host_helpers.h"

namespace dsk_tools {

    // Initialize static callback pointer
    bool (*fsHost::use_recycle_bin)() = nullptr;

    // Helper function to sanitize filenames by replacing invalid characters
    static std::string sanitize_filename(const std::string& filename)
    {
        std::string sanitized = filename;

#ifdef _WIN32
        // Windows invalid characters: < > : " / \ | ? *
        const std::string invalid_chars = "<>:\"/\\|?*";
#else
        // POSIX: mainly / is invalid, but we also avoid control characters
        const std::string invalid_chars = "/";
#endif

        // Replace invalid characters with underscores
        for (size_t i = 0; i < sanitized.length(); ++i) {
            unsigned char c = static_cast<unsigned char>(sanitized[i]);

            // Check for invalid characters in the predefined set
            if (invalid_chars.find(c) != std::string::npos) {
                sanitized[i] = '_';
            }
            // Also replace control characters (0-31) on all platforms
            else if (c < 32) {
                sanitized[i] = '_';
            }
        }

        return sanitized;
    }

    fsHost::fsHost(diskImage * image):
        fileSystem(nullptr)
    {}

    FSCaps fsHost::get_caps()
    {
        return FSCaps::Delete | FSCaps::Add | FSCaps::Dirs | FSCaps::Rename| FSCaps::MkDir;
    }

    Result fsHost::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);
        is_open = true;
        return Result::ok();
    }

    std::vector<std::string> fsHost::get_save_file_formats()
    {
        return {};
    }

    Result fsHost::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (uf.fs != get_fs()) return Result::error(ErrorCode::FileIncorrectFS);

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
        // Sanitize filename to remove invalid characters for the target OS
        std::string sanitized_name = sanitize_filename(uf.name);

        std::cout << "Host: put_file " << m_path << " + " << sanitized_name << std::endl;

        // Construct full file path
        std::string fullPath = join_paths(m_path, sanitized_name);

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
            return Result::error(ErrorCode::CreateError);
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
        if (uf.fs != get_fs()) return Result::error(ErrorCode::FileIncorrectFS);

        // Extract path from metadata
        std::string path = bytesToString(uf.metadata);
        std::cout << "Host: delete_file " << path << std::endl;

        // Check if file exists
        UTF8_ifstream testFile(path);
        if (!testFile.good()) {
            return Result::error(ErrorCode::FileNotFound);
        }
        testFile.close();

        // Check if recycle bin is enabled
        bool use_trash = (use_recycle_bin != nullptr && use_recycle_bin());

        if (use_trash) {
            // Try to move to trash first
            if (utf8_trash(path) == 0) {
                return Result::ok();
            }
            // Trash failed - return special error for UI to handle
            return Result::error(ErrorCode::FileDeleteError, "TRASH_FAILED");
        }

        // Permanent deletion
        if (utf8_remove(path) != 0) {
            return Result::error(ErrorCode::FileDeleteError);
        }

        return Result::ok();
    }

    Result fsHost::dir(std::vector<dsk_tools::UniversalFile> & files, bool show_deleted)
    {
        files.clear();

        if (m_path.empty()) {
            return Result::error(ErrorCode::DirError);
        }

        bool at_root = is_at_root(m_path);

        // Add parent directory entry if not at root
        if (!at_root) {
            UniversalFile parent;
            parent.fs = FS::Host;
            parent.name = "..";
            parent.is_dir = true;
            parent.size = 0;
            parent.is_protected = false;
            parent.is_deleted = false;
            parent.type_preferred = PreferredType::Binary;

            std::string parent_path = get_parent_path(m_path);
            parent.metadata = strToBytes(parent_path);

            files.push_back(parent);
        }

#ifdef _WIN32
        // Windows implementation using _wfindfirst/_wfindnext
        std::wstring search_pattern = utf8_to_wide(m_path);
        if (!search_pattern.empty() && search_pattern.back() != L'\\' && search_pattern.back() != L'/') {
            search_pattern += L'\\';
        }
        search_pattern += L'*';

        struct _wfinddata_t fileInfo;
        intptr_t handle = _wfindfirst(search_pattern.c_str(), &fileInfo);

        if (handle != -1) {
            do {
                std::wstring wname = fileInfo.name;

                // Convert wide filename to UTF-8
                int utf8_length = WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1,
                                                     nullptr, 0, nullptr, nullptr);
                if (utf8_length <= 0) continue;

                std::string name(utf8_length - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1,
                                   &name[0], utf8_length, nullptr, nullptr);

                // Skip "." and ".." as we handle parent manually
                if (name == "." || name == "..") continue;

                UniversalFile uf;
                uf.fs = FS::Host;
                uf.name = name;
                uf.is_dir = (fileInfo.attrib & _A_SUBDIR) != 0;
                uf.size = uf.is_dir ? 0 : static_cast<uint32_t>(fileInfo.size);
                uf.is_protected = false;
                uf.is_deleted = false;
                uf.type_preferred = PreferredType::Binary;

                // Construct full path for metadata
                std::string fullPath = join_paths(m_path, name);

                uf.metadata = strToBytes(fullPath);
                files.push_back(uf);

            } while (_wfindnext(handle, &fileInfo) == 0);

            _findclose(handle);
        }

#else
        // POSIX implementation using opendir/readdir/stat
        DIR* dir = opendir(m_path.c_str());
        if (!dir) {
            return Result::ok();  // Empty or inaccessible directory
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;

            // Skip "." and ".." as we handle parent manually
            if (name == "." || name == "..") continue;

            // Construct full path
            std::string fullPath = join_paths(m_path, name);

            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) != 0) {
                continue;  // Skip files we can't stat
            }

            UniversalFile uf;
            uf.fs = FS::Host;
            uf.name = name;
            uf.is_dir = S_ISDIR(statbuf.st_mode);
            uf.size = uf.is_dir ? 0 : static_cast<uint32_t>(statbuf.st_size);
            uf.is_protected = false;
            uf.is_deleted = false;
            uf.type_preferred = PreferredType::Binary;

            uf.metadata = strToBytes(fullPath);
            files.push_back(uf);
        }

        closedir(dir);
#endif

        return Result::ok();
    }

    void fsHost::cd(const std::string & path, bool & updir)
    {
        m_path = path;
        updir = false;
    }

    void fsHost::cd(const UniversalFile & dir, bool & updir)
    {
        std::string fullPath = join_paths(m_path, dir.name);
        cd(fullPath, updir);
    }

    void fsHost::cd_up()
    {
        if (!is_at_root(m_path)) {
            m_path = get_parent_path(m_path);
        }
    }

    Result fsHost::mkdir(const std::string & dir_name, UniversalFile & new_dir)
    {
        // Sanitize directory name to remove invalid characters for the target OS
        std::string sanitized_name = sanitize_filename(dir_name);

        // Construct full path
        std::string fullPath = join_paths(m_path, sanitized_name);

        // Platform-specific directory creation with UTF-8 support
        int result = utf8_mkdir(fullPath);

        if (result != 0) {
            // Check if directory already exists
            if (errno == EEXIST) {
                return Result::error(ErrorCode::DirAlreadyExists);
            }
            if (errno == EINVAL) {
                return Result::error(ErrorCode::InvalidName);
            }
            return Result::error(ErrorCode::DirError);
        }

        new_dir.fs = get_fs();
        new_dir.name = dir_name;
        new_dir.is_dir = true;
        new_dir.metadata = strToBytes(fullPath);

        return Result::ok();
    }

    Result fsHost::mkdir(const UniversalFile & uf, UniversalFile & new_dir)
    {
        return mkdir(uf.name, new_dir);
    }

    Result fsHost::rename_file(const UniversalFile & fd, const std::string & new_name)
    {
        // Validate filesystem type
        if (fd.fs != get_fs()) return Result::error(ErrorCode::FileIncorrectFS);

        // Extract path from metadata
        std::string oldPath = bytesToString(fd.metadata);
        std::cout << "Host: rename_file " << oldPath << " -> " << new_name << std::endl;

        // Get directory of the file and construct new path
        std::string dirPath = get_parent_path(oldPath);
        std::string newPath = join_paths(dirPath, new_name);

        // Check if old file exists
        UTF8_ifstream testFile(oldPath);
        if (!testFile.good()) {
            testFile.close();
            return Result::error(ErrorCode::FileNotFound);
        }
        testFile.close();

        // Check if new file already exists
        UTF8_ifstream testNewFile(newPath);
        if (testNewFile.good()) {
            testNewFile.close();
            return Result::error(ErrorCode::FileAlreadyExists);
        }
        testNewFile.close();

#ifdef _WIN32
        // Windows implementation using _wrename for UTF-8 support
        std::wstring wOldPath = utf8_to_wide(oldPath);
        std::wstring wNewPath = utf8_to_wide(newPath);

        if (_wrename(wOldPath.c_str(), wNewPath.c_str()) != 0) {
            if (errno == EINVAL) {
                return Result::error(ErrorCode::InvalidName);
            }
            return Result::error(ErrorCode::FileRenameError);
        }
#else
        // POSIX implementation using rename
        if (std::rename(oldPath.c_str(), newPath.c_str()) != 0) {
            if (errno == EINVAL) {
                return Result::error(ErrorCode::InvalidName);
            }
            return Result::error(ErrorCode::FileRenameError);
        }
#endif

        return Result::ok();
    }

}
