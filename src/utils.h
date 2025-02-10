#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <iomanip>
#include <ios>
#include <string>

namespace dsk_tools
{
    std::string koi7_to_utf(const uint8_t in[], int len);
    std::string trim(const std::string& str, const std::string& whitespace = " \t");
    std::string get_file_ext(const std::string &file_name);

    template< typename T >
    std::string int_to_hex( T i )
    {
        std::stringstream stream;
        stream << std::uppercase
               << std::setfill ('0') << std::setw(sizeof(T)*2)
               << std::hex << static_cast<unsigned int>(i);
        return stream.str();
    }

    std::string toBCD(uint8_t b);

}
#endif // UTILS_H
