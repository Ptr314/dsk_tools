#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>

#include "dsk_tools/definitions.h"

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

    std::string trim(const std::string& str,
                     const std::string& whitespace = " \t")
    {
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
            return ""; // no content

        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }
}
#endif // UTILS_H
