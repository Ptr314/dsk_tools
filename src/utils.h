#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>

namespace dsk_tools
{
std::string koi7_to_utf(const uint8_t in[], int len);

    std::string trim(const std::string& str, const std::string& whitespace = " \t");
}
#endif // UTILS_H
