#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H

#include <string>
#include "dsk_tools/disk_image.h"

namespace dsk_tools {

     dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id);

} // namespace

#endif /* DSK_TOOLS_H */
