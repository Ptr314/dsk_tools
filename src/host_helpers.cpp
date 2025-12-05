// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Platform-specific UTF-8 file I/O helpers for host filesystem

#include "host_helpers.h"
#include "utils.h"

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include <shellapi.h>
    #include <vector>

    #ifdef _MSC_VER
        // MSVC: Use std::ifstream with wchar_t* overload (Microsoft extension)
        UTF8_ifstream::UTF8_ifstream(const std::string& filename,
                                      std::ios_base::openmode mode)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            this->open(wpath.c_str(), mode);
        }

        UTF8_ofstream::UTF8_ofstream(const std::string& filename,
                                      std::ios_base::openmode mode)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            this->open(wpath.c_str(), mode);
        }

    #else
        // MinGW and other compilers: Use Windows API directly for UTF-8 path support

        UTF8_ifstream::UTF8_ifstream(const std::string& filename,
                                      std::ios_base::openmode mode)
            : m_handle(INVALID_HANDLE_VALUE), m_state(std::ios_base::goodbit)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            m_handle = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
            }
        }

        UTF8_ifstream::~UTF8_ifstream() {
            if (m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
            }
        }

        bool UTF8_ifstream::is_open() const {
            return m_handle != INVALID_HANDLE_VALUE;
        }

        std::streamsize UTF8_ifstream::read(char* buffer, std::streamsize count) {
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
                return 0;
            }

            DWORD bytesRead = 0;
            if (!ReadFile(m_handle, buffer, static_cast<DWORD>(count), &bytesRead, nullptr)) {
                m_state = std::ios_base::failbit;
                return 0;
            }
            return static_cast<std::streamsize>(bytesRead);
        }

        void UTF8_ifstream::seekg(std::streamoff offset, std::ios_base::seekdir way) {
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
                return;
            }

            DWORD dwMoveMethod = FILE_BEGIN;
            if (way == std::ios_base::end) {
                dwMoveMethod = FILE_END;
            } else if (way == std::ios_base::cur) {
                dwMoveMethod = FILE_CURRENT;
            }

            if (!SetFilePointer(m_handle, static_cast<LONG>(offset), nullptr, dwMoveMethod)) {
                m_state = std::ios_base::failbit;
            }
        }

        std::streampos UTF8_ifstream::tellg() {
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
                return -1;
            }

            DWORD pos = SetFilePointer(m_handle, 0, nullptr, FILE_CURRENT);
            if (pos == INVALID_SET_FILE_POINTER) {
                m_state = std::ios_base::failbit;
                return -1;
            }
            return static_cast<std::streampos>(pos);
        }

        void UTF8_ifstream::close() {
            if (m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        bool UTF8_ifstream::good() const {
            return m_handle != INVALID_HANDLE_VALUE && (m_state & std::ios_base::failbit) == 0;
        }

        UTF8_ifstream::operator bool() const {
            return good();
        }

        UTF8_ofstream::UTF8_ofstream(const std::string& filename,
                                      std::ios_base::openmode mode)
            : m_handle(INVALID_HANDLE_VALUE), m_state(std::ios_base::goodbit)
        {
            std::wstring wpath = dsk_tools::utf8_to_wide(filename);
            DWORD dwCreationDisposition = CREATE_ALWAYS;
            if (mode & std::ios_base::app) {
                dwCreationDisposition = OPEN_ALWAYS;
            }

            m_handle = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0,
                                  nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
            }

            if ((mode & std::ios_base::app) && m_handle != INVALID_HANDLE_VALUE) {
                SetFilePointer(m_handle, 0, nullptr, FILE_END);
            }
        }

        UTF8_ofstream::~UTF8_ofstream() {
            if (m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
            }
        }

        bool UTF8_ofstream::is_open() const {
            return m_handle != INVALID_HANDLE_VALUE;
        }

        void UTF8_ofstream::write(const char* buffer, std::streamsize count) {
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_state = std::ios_base::failbit;
                return;
            }

            DWORD bytesWritten = 0;
            if (!WriteFile(m_handle, buffer, static_cast<DWORD>(count), &bytesWritten, nullptr)) {
                m_state = std::ios_base::failbit;
            }
        }

        void UTF8_ofstream::close() {
            if (m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        bool UTF8_ofstream::good() const {
            return m_handle != INVALID_HANDLE_VALUE && (m_state & std::ios_base::failbit) == 0;
        }

        UTF8_ofstream::operator bool() const {
            return good();
        }

    #endif

    // Helper function for file removal with UTF-8 path
    int utf8_remove(const std::string& path) {
        std::wstring wpath = dsk_tools::utf8_to_wide(path);
        return _wremove(wpath.c_str());
    }

    // Helper function for directory creation with UTF-8 path
    int utf8_mkdir(const std::string& path) {
        std::wstring wpath = dsk_tools::utf8_to_wide(path);
        return _wmkdir(wpath.c_str());
    }

    // Recycle bin / Trash functionality
    int utf8_trash(const std::string& path) {
        std::wstring wpath = dsk_tools::utf8_to_wide(path);

        // SHFileOperation requires double-null terminated string
        std::vector<wchar_t> pathBuffer(wpath.size() + 2, 0);
        std::copy(wpath.begin(), wpath.end(), pathBuffer.begin());

        SHFILEOPSTRUCTW fileOp = {};
        fileOp.hwnd = NULL;
        fileOp.wFunc = FO_DELETE;
        fileOp.pFrom = pathBuffer.data();
        fileOp.pTo = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
        fileOp.fAnyOperationsAborted = FALSE;

        int result = SHFileOperationW(&fileOp);
        return (result == 0 && !fileOp.fAnyOperationsAborted) ? 0 : -1;
    }

#elif defined(__APPLE__)
    #include <cstdio>
    #include <sys/stat.h>
    #import <Foundation/Foundation.h>

    int utf8_remove(const std::string& path) {
        return std::remove(path.c_str());
    }

    int utf8_mkdir(const std::string& path) {
        return ::mkdir(path.c_str(), 0755);
    }

    // macOS-specific trash implementation
    int utf8_trash(const std::string& path) {
        @autoreleasepool {
            NSString *nsPath = [NSString stringWithUTF8String:path.c_str()];
            NSURL *fileURL = [NSURL fileURLWithPath:nsPath];

            NSFileManager *fileManager = [NSFileManager defaultManager];
            NSError *error = nil;

            BOOL success = [fileManager trashItemAtURL:fileURL
                                      resultingItemURL:nil
                                                 error:&error];

            return success ? 0 : -1;
        }
    }

#else  // Linux
    #include <cstdio>
    #include <sys/stat.h>
    #include <ctime>
    #include <cstring>
    #include <cstdlib>

    int utf8_remove(const std::string& path) {
        return std::remove(path.c_str());
    }

    int utf8_mkdir(const std::string& path) {
        return ::mkdir(path.c_str(), 0755);
    }

    // Recycle bin / Trash functionality
    namespace {
        std::string get_trash_dir() {
            const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
            std::string trash_dir;

            if (xdg_data_home && xdg_data_home[0] == '/') {
                trash_dir = std::string(xdg_data_home) + "/Trash";
            } else {
                const char* home = std::getenv("HOME");
                if (home) {
                    trash_dir = std::string(home) + "/.local/share/Trash";
                } else {
                    return "";
                }
            }

            // Ensure trash directories exist
            utf8_mkdir(trash_dir);
            utf8_mkdir(trash_dir + "/files");
            utf8_mkdir(trash_dir + "/info");

            return trash_dir;
        }
    }

    int utf8_trash(const std::string& path) {
        std::string trash_dir = get_trash_dir();
        if (trash_dir.empty()) return -1;

        // Extract filename from path
        size_t last_slash = path.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);

        std::string trash_file = trash_dir + "/files/" + filename;
        std::string trash_info = trash_dir + "/info/" + filename + ".trashinfo";

        // Handle name collision with timestamp
        struct stat st;
        if (stat(trash_file.c_str(), &st) == 0) {
            time_t now = time(nullptr);
            char timestamp[32];
            snprintf(timestamp, sizeof(timestamp), ".%ld", (long)now);
            trash_file += timestamp;
            trash_info = trash_dir + "/info/" + filename + timestamp + ".trashinfo";
        }

        // Move file to trash
        if (std::rename(path.c_str(), trash_file.c_str()) != 0) {
            return -1;
        }

        // Create .trashinfo file
        UTF8_ofstream info(trash_info);
        if (!info.good()) {
            // Restore file on failure
            std::rename(trash_file.c_str(), path.c_str());
            return -1;
        }

        info.write("[Trash Info]\n", 13);
        std::string path_line = "Path=" + path + "\n";
        info.write(path_line.c_str(), path_line.size());

        time_t now = time(nullptr);
        struct tm* tm_info = localtime(&now);
        char date_buf[32];
        strftime(date_buf, sizeof(date_buf), "DeletionDate=%Y-%m-%dT%H:%M:%S\n", tm_info);
        info.write(date_buf, strlen(date_buf));

        info.close();
        return 0;
    }

#endif