// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Platform-specific UTF-8 file I/O helpers for host filesystem
#pragma once

#include <fstream>
#include <ios>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <io.h>

    // Windows-specific UTF-8 file I/O wrappers
    #ifdef _MSC_VER
        // MSVC: Use std::ifstream with wchar_t* overload (Microsoft extension)
        class UTF8_ifstream : public std::ifstream {
        public:
            explicit UTF8_ifstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::in)
            {
                std::wstring wpath = dsk_tools::utf8_to_wide(filename);
                this->open(wpath.c_str(), mode);
            }
        };

        class UTF8_ofstream : public std::ofstream {
        public:
            explicit UTF8_ofstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::out)
            {
                std::wstring wpath = dsk_tools::utf8_to_wide(filename);
                this->open(wpath.c_str(), mode);
            }
        };

    #else
        // MinGW and other compilers: Use Windows API directly for UTF-8 path support

        class UTF8_ifstream {
        private:
            HANDLE m_handle;
            std::ios_base::iostate m_state;

        public:
            explicit UTF8_ifstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::in)
                : m_handle(INVALID_HANDLE_VALUE), m_state(std::ios_base::goodbit)
            {
                std::wstring wpath = dsk_tools::utf8_to_wide(filename);
                m_handle = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (m_handle == INVALID_HANDLE_VALUE) {
                    m_state = std::ios_base::failbit;
                }
            }

            ~UTF8_ifstream() {
                if (m_handle != INVALID_HANDLE_VALUE) {
                    CloseHandle(m_handle);
                }
            }

            bool is_open() const {
                return m_handle != INVALID_HANDLE_VALUE;
            }

            std::streamsize read(char* buffer, std::streamsize count) {
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

            void seekg(std::streamoff offset, std::ios_base::seekdir way) {
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

            std::streampos tellg() {
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

            void close() {
                if (m_handle != INVALID_HANDLE_VALUE) {
                    CloseHandle(m_handle);
                    m_handle = INVALID_HANDLE_VALUE;
                }
            }

            bool good() const {
                return m_handle != INVALID_HANDLE_VALUE && (m_state & std::ios_base::failbit) == 0;
            }

            operator bool() const {
                return good();
            }
        };

        class UTF8_ofstream {
        private:
            HANDLE m_handle;
            std::ios_base::iostate m_state;

        public:
            explicit UTF8_ofstream(const std::string& filename,
                                  std::ios_base::openmode mode = std::ios_base::out)
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

            ~UTF8_ofstream() {
                if (m_handle != INVALID_HANDLE_VALUE) {
                    CloseHandle(m_handle);
                }
            }

            bool is_open() const {
                return m_handle != INVALID_HANDLE_VALUE;
            }

            void write(const char* buffer, std::streamsize count) {
                if (m_handle == INVALID_HANDLE_VALUE) {
                    m_state = std::ios_base::failbit;
                    return;
                }

                DWORD bytesWritten = 0;
                if (!WriteFile(m_handle, buffer, static_cast<DWORD>(count), &bytesWritten, nullptr)) {
                    m_state = std::ios_base::failbit;
                }
            }

            void close() {
                if (m_handle != INVALID_HANDLE_VALUE) {
                    CloseHandle(m_handle);
                    m_handle = INVALID_HANDLE_VALUE;
                }
            }

            bool good() const {
                return m_handle != INVALID_HANDLE_VALUE && (m_state & std::ios_base::failbit) == 0;
            }

            operator bool() const {
                return good();
            }
        };

    #endif

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