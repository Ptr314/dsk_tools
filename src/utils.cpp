// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Service functions

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <string>
#include <unordered_map>
#include <fstream>

#include "utils.h"
#include "charmaps.h"
#include "definitions.h"

namespace dsk_tools
{
    std::string agat_to_utf(const uint8_t in[], int len)
    {
        std::string out;
        for (int i=0; i < len; i ++) {
            out.append(dsk_tools::agat_charmap[in[i]]);
        }

        return out;
    }

    // std::string ascii_to_agat(const std::string & in)
    // {
    //     std::string out;
    //     for (int i=0; i < in.size(); i ++) {
    //         out.push_back(static_cast<char>(in[i] | 0x80));
    //     }
    //     return out;

    // }

    std::vector<std::string> split_utf8_chars(const std::string& str) {
        std::vector<std::string> result;
        for (size_t i = 0; i < str.size();) {
            unsigned char c = str[i];
            size_t len = 1;
            if ((c & 0x80) == 0x00) len = 1;        // 1-byte character
            else if ((c & 0xE0) == 0xC0) len = 2;   // 2-byte character
            else if ((c & 0xF0) == 0xE0) len = 3;   // 3-byte character
            else if ((c & 0xF8) == 0xF0) len = 4;   // 4-byte character

            result.push_back(str.substr(i, len));
            i += len;
        }
        return result;
    }

    std::vector<uint8_t> utf_to_agat(const std::string& input) {
        auto& map = get_reverse_agat_charmap();

        std::vector<uint8_t> output;

        std::vector<std::string> chars = split_utf8_chars(input);

        for (const auto& ch : chars) {
            auto it = map.find(ch);
            if (it != map.end()) {
                output.push_back(static_cast<uint8_t>(it->second));
            } else {
                // Код символа "?" (в agat_charmap это символ с кодом 175)
                output.push_back(175);  // '?'
            }
        }

        return output;
    }

    std::string trim(const std::string& str, const std::string& whitespace)
    {
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
            return ""; // no content

        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    std::string get_file_ext(const std::string &file_name) {
        size_t dot_pos = file_name.find_last_of('.');
        if (dot_pos == std::string::npos || dot_pos == 0) {
            return "";
        }
        std::string ext = file_name.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    std::string get_filename(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos)
            return path;
        return path.substr(pos + 1);
    }

    std::string get_file_path(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos)
            return path;
        return path.substr(0, pos+1);
    }

    std::string toBCD(uint8_t byte) {
        return std::to_string(byte >> 4) + std::to_string(byte & 0xF);
    }


    constexpr char BASE64_CHARS[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string base64_encode(const std::vector<uint8_t>& data, size_t line_length_limit) {
        std::string encoded;
        int val = 0;
        int valb = -6;
        size_t line_length = 0; // Счетчик символов в строке

        for (uint8_t c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                encoded.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
                valb -= 6;
                line_length++;

                // Если установлен лимит строки и он достигнут, добавляем перевод строки
                if (line_length_limit > 0 && line_length == line_length_limit) {
                    encoded.push_back('\n');
                    line_length = 0;
                }
            }
        }

        if (valb > -6) {
            encoded.push_back(BASE64_CHARS[((val << (6 + valb)) & 0x3F)]);
            line_length++;
        }

        while (encoded.size() % 4) {
            encoded.push_back('=');
            line_length++;

            // Учитываем лимит строки при добавлении '='
            if (line_length_limit > 0 && line_length == line_length_limit) {
                encoded.push_back('\n');
                line_length = 0;
            }
        }

        return encoded;
    }

    std::vector<uint8_t> base64_decode(const std::string& encoded) {
        // Таблица для обратного поиска Base64 символов
        std::unordered_map<char, uint8_t> base64_map;
        for (size_t i = 0; i < 64; ++i) {
            base64_map[BASE64_CHARS[i]] = static_cast<uint8_t>(i);
        }

        std::vector<uint8_t> decoded;
        int val = 0;
        int valb = -8;

        for (char c : encoded) {
            if (c == '=' || c == '\n' || c == '\r') {
                continue; // Игнорируем символы перевода строки и '=' (заполнитель)
            }
            if (base64_map.find(c) == base64_map.end()) {
                throw std::invalid_argument("Incorrect base64 character");
            }

            val = (val << 6) + base64_map[c];
            valb += 6;

            if (valb >= 0) {
                decoded.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return decoded;
    }

    std::string toHexList(const std::vector<uint8_t> & data, std::string prefix)
    {
        return toHexList(static_cast<const uint8_t*>(data.data()), data.size(), prefix);
    }

    std::string toHexList(const uint8_t *data, int len, std::string prefix)
    {
        std::string out = "";
        for (int i=0; i < len; i++) {
            uint8_t b = data[i];
            if (out.size() > 0) out += " ";
            out += prefix;
            out += int_to_hex(b);
        }
        return out;
    }

    bool iterate_until(const std::vector<uint8_t> & in, int & p, const uint8_t v)
    {
        uint8_t d;
        do {
            if (p >= in.size()) return false;
            d = in.at(p++);
        } while (d != v);
        return true;
    }

    int agat_attr_to_type(uint8_t a)
    {
        uint8_t v = a & 0x7F;
        int n = 0;
        do {
            if (v == 0) return n;
            v >>= 1;
            n++;
        } while (n < 8);
        return 0;
    }

    int agat_preferred_file_type(int t)
    {
        if (t == 0)
            return PREFERRED_TEXT;
        else
        if (t == 2)
            return PREFERRED_AGATBASIC;
        else
            return PREFERRED_BINARY;
    }

    std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    bool file_exists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }

    std::string pad_number(int num, size_t width) {
        std::string s = std::to_string(num);
        if (s.length() >= width) {
            return s;
        }
        return std::string(width - s.length(), ' ') + s;
    }

} // namespace
