#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H

#include <string>

#include "definitions.h"
#include "utils.h"

#include "disk_image.h"
#include "image_agat140.h"
#include "image_agat840.h"

#include "loader.h"
#include "loader_raw.h"

#include "writer.h"
#include "writer_raw.h"
#include "writer_mfm.h"
#include "writer_hxc_hfe.h"
#include "writer_hxc_mfm.h"

#include "filesystem.h"
#include "fs_dos33.h"
#include "fs_spriteos.h"

namespace dsk_tools {

int detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id);
    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id);
    dsk_tools::fileSystem * prepare_filesystem(dsk_tools::diskImage * image, std::string filesystem_id);
    BYTES code44(const BYTES & buffer);
    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out);

} // namespace

#endif /* DSK_TOOLS_H */
