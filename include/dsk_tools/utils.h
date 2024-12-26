#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>

#include "dsk_tools/definitions.h"

namespace dsk_tools
{
    std::string koi7_to_utf(uint8_t in[], int len)
    {
        std::string out;
        for (int i=0; i < len; i ++) {
            out.append(koi7map[in[i]]);
        }

        return out;
    }
}
#endif // UTILS_H
