#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H

#include <string>
#include "dsk_tools/definitions.h"
#include "dsk_tools/disk_image.h"
#include "dsk_tools/image_agat140.h"
#include "dsk_tools/writer_hxc_hfe.h"
#include "dsk_tools/writer_hxc_mfm.h"
#include "dsk_tools/writer_raw.h"

namespace dsk_tools {

    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id);
    BYTES code44(const BYTES & buffer);
    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out);

} // namespace

#endif /* DSK_TOOLS_H */
