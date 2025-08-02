// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Service functions

#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <iomanip>
#include <ios>
#include <string>
#include <vector>
#include <sstream>

namespace dsk_tools
{
    std::string agat_to_utf(const uint8_t in[], int len);
    std::string trim(const std::string& str, const std::string& whitespace = " \t");
    std::string get_file_ext(const std::string &file_name);
    std::string get_filename(const std::string& path);
    std::string get_file_path(const std::string& path);
    std::vector<std::string> split_utf8_chars(const std::string& str);
    std::vector<uint8_t> utf_to_agat(const std::string& input);

    template< typename T >
    std::string int_to_hex( T i, bool fill = true )
    {
        std::stringstream stream;
        stream << std::uppercase;
        if (fill) stream << std::setfill ('0') << std::setw(sizeof(T)*2);
        stream << std::hex << static_cast<unsigned int>(i);
        return stream.str();
    }

    template< typename T >
    std::string int_to_octal( T i )
    {
        std::stringstream stream;
        stream << std::uppercase;
        stream << std::oct << static_cast<unsigned int>(i);
        return stream.str();
    }

    std::string toBCD(uint8_t b);
    std::string toHexList(const std::vector<uint8_t> & data, std::string prefix = "");
    std::string toHexList(const uint8_t *data, int len, std::string prefix = "");

    std::string base64_encode(const std::vector<uint8_t>& data, size_t line_length_limit = 0);
    std::vector<uint8_t> base64_decode(const std::string& encoded);
    bool iterate_until(const std::vector<uint8_t> & in, int & p, const uint8_t v);

    int agat_attr_to_type(uint8_t a);
    int agat_preferred_file_type(int t);
    std::string to_upper(std::string s);
    bool file_exists(const std::string& filename);
    std::string pad_number(int num, size_t width = 4);

}
#endif // UTILS_H
