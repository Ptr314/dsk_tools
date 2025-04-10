#ifndef IMAGE_FIL_H
#define IMAGE_FIL_H

#include "disk_image.h"

namespace dsk_tools {

    class imageFIL: public diskImage
    {
    protected:

    public:
        imageFIL(Loader * loader);
        virtual int check() override;
    };
}

#endif // IMAGE_FIL_H
