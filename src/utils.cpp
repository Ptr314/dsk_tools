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

    std::string ascii_to_agat(const std::string & in)
    {
        std::string out;
        for (int i=0; i < in.size(); i ++) {
            out.push_back(static_cast<char>(in[i] | 0x80));
        }
        return out;

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

} // namespace
