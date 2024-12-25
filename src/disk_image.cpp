#include "dsk_tools/disk_image.h"

namespace dsk_tools {

    diskImage::diskImage(Loader * loader):
        loader(loader)
    {

    }
    int diskImage::load()
    {
        return loader->load(&buffer);
    }

}
