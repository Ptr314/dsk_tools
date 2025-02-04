#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>

namespace dsk_tools
{
    std::string koi7_to_utf(const uint8_t in[], int len);
    std::string trim(const std::string& str, const std::string& whitespace = " \t");
    std::string get_file_ext(const std::string &file_name);
}
#endif // UTILS_H
