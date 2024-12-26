#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H

#include <string>
#include "dsk_tools/disk_image.h"

namespace dsk_tools {

    #define FDD_LOAD_OK                 0
    #define FDD_LOAD_ERROR              1
    #define FDD_LOAD_SIZE_SMALLER       2
    #define FDD_LOAD_SIZE_LARGER        3
    #define FDD_LOAD_PARAMS_MISMATCH    4
    #define FDD_LOAD_INCORRECT_FILE     5
    #define FDD_LOAD_FILE_CORRUPT       6

    #define FDD_OPEN_OK                 0
    #define FDD_OPEN_BAD_FORMAT         1

     dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id);

} // namespace

#endif /* DSK_TOOLS_H */
