#include "viewer_basic_apple.h"
#include "bas_tokens.h"

namespace dsk_tools {
    ViewerRegistrar<ViewerBASIC_Apple> ViewerBASIC_Apple::registrar;

    std::string ViewerBASIC_Apple::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        return convert_tokenized(data, cm_name, dsk_tools::ABS_tokens);
    }
}
