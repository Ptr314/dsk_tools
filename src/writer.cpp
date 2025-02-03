#include "writer.h"

namespace dsk_tools {

    Writer::Writer(const std::string & format_id, diskImage * image_to_save):
              format_id(format_id)
            , image(image_to_save)
        {}

    Writer::~Writer()
        {}

}
