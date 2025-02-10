#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <string>

#include "definitions.h"
#include "utils.h"

namespace dsk_tools
{
    std::string koi7_to_utf(const uint8_t in[], int len)
    {
        std::string out;
        for (int i=0; i < len; i ++) {
            out.append(koi7map[in[i] & 0x7F]);
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

    std::string toBCD(uint8_t byte) {
        return std::to_string(byte >> 4) + std::to_string(byte & 0xF);
    }

} // namespace
