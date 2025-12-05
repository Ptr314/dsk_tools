// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Platform-specific UTF-8 file I/O helpers for host filesystem
#pragma once

#include <fstream>
#include <ios>
#include <string>

// UTF-8 aware file stream classes for cross-platform use
#ifdef _WIN32
    // Windows: UTF-8 file I/O wrappers
    #ifdef _MSC_VER
        class UTF8_ifstream : public std::ifstream {
        public:
            explicit UTF8_ifstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::in);
        };

        class UTF8_ofstream : public std::ofstream {
        public:
            explicit UTF8_ofstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::out);
        };

    #else
        // MinGW and other compilers: Use Windows API directly for UTF-8 path support
        class UTF8_ifstream {
        private:
            void* m_handle;  // HANDLE
            int m_state;     // std::ios_base::iostate

        public:
            explicit UTF8_ifstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::in);
            ~UTF8_ifstream();

            bool is_open() const;
            std::streamsize read(char* buffer, std::streamsize count);
            void seekg(std::streamoff offset, std::ios_base::seekdir way);
            std::streampos tellg();
            void close();
            bool good() const;
            operator bool() const;
        };

        class UTF8_ofstream {
        private:
            void* m_handle;  // HANDLE
            int m_state;     // std::ios_base::iostate

        public:
            explicit UTF8_ofstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::out);
            ~UTF8_ofstream();

            bool is_open() const;
            void write(const char* buffer, std::streamsize count);
            void close();
            bool good() const;
            operator bool() const;
        };

    #endif

    // Helper functions for file operations with UTF-8 path
    int utf8_remove(const std::string& path);
    int utf8_mkdir(const std::string& path);
    int utf8_trash(const std::string& path);

#elif defined(__APPLE__)
    // On macOS, use standard classes directly
    using UTF8_ifstream = std::ifstream;
    using UTF8_ofstream = std::ofstream;

    int utf8_remove(const std::string& path);
    int utf8_mkdir(const std::string& path);
    int utf8_trash(const std::string& path);

#else  // Linux
    // On non-Windows platforms, use standard classes directly
    using UTF8_ifstream = std::ifstream;
    using UTF8_ofstream = std::ofstream;

    int utf8_remove(const std::string& path);
    int utf8_mkdir(const std::string& path);
    int utf8_trash(const std::string& path);

#endif