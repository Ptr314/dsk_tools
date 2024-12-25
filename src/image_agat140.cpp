
#include "dsk_tools/loader.h"
#include "dsk_tools/image_agat140.h"

namespace dsk_tools {
    imageAgat140::imageAgat140(Loader * loader):
        diskImage(loader)
    {}

    int imageAgat140::check()
    {
        return FDD_LOAD_OK;
    }
}
