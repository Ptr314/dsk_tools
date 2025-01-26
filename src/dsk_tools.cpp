#include <iostream>
#include "dsk_tools/dsk_tools.h"
#include "dsk_tools/loader_raw.h"
#include "dsk_tools/image_agat140.h"
#include "dsk_tools/image_agat840.h"

namespace dsk_tools {

    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id)
    {
        std::cout << file_name << std::endl;
        dsk_tools::LoaderRAW * loader;
        if (format_id == "FILE_RAW_MSB") {
            loader = new dsk_tools::LoaderRAW(file_name, format_id, type_id, true);
        } else {
            return nullptr;
        }

        if (type_id == "TYPE_AGAT_140") {
            dsk_tools::imageAgat140 * disk_image = new dsk_tools::imageAgat140(loader);
            return disk_image;
        } else
        if (type_id == "TYPE_AGAT_840") {
            dsk_tools::imageAgat840 * disk_image = new dsk_tools::imageAgat840(loader);
            return disk_image;
        }

        return nullptr;
    }

} // namespace
