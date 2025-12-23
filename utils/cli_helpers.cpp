// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: FDD image conversion utility command-line tool

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdexcept>
#include <climits>

#include "host_helpers.h"
#include "cli_helpers.h"

namespace dsk_tools {

    void setupConsole() {
        #ifdef _WIN32
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode;
            if (GetConsoleMode(hOut, &mode)) {
                // ENABLE_VIRTUAL_TERMINAL_PROCESSING not available in older Windows SDK versions
        #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
        #endif
                SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        #endif
    }

    Result write_output_file(const std::string & output_file, const std::string & format_id, const uint8_t volume_id, diskImage * image, const bool verbose)
    {
        if (verbose) std::cout << "Writing to output: " << output_file << std::endl;

        std::string out_format_id;
        std::string type_id;
        std::string filesystem_id;
        Result res = detect_fdd_type(output_file, out_format_id, type_id, filesystem_id, true);
        if (!res) return res;

        const auto writer = create_writer(out_format_id, volume_id, image);
        if (!writer) { return Result::error(ErrorCode::WriteError); }

        BYTES buffer;
        const Result write_res = writer->write(buffer);
        if (!write_res) return write_res;
        UTF8_ofstream file(output_file, std::ios::binary);
        if (file.good()) {
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (file.good()) return Result::ok();
        }
        return Result::error(ErrorCode::WriteError);
    }

    unsigned int parse_number(const std::string& str) {
        if (str.empty()) {
            throw std::invalid_argument("Empty string cannot be parsed as a number");
        }

        unsigned long value;

        if (str[0] == '$') {
            if (str.length() < 2) {
                throw std::invalid_argument("Invalid hex number format");
            }
            value = std::stoul(str.substr(1), nullptr, 16);
        } else {
            value = std::stoul(str, nullptr, 10);
        }

        if (value > UINT_MAX) {
            throw std::out_of_range("Number is too large to fit in unsigned int");
        }

        return static_cast<unsigned int>(value);
    }
}
