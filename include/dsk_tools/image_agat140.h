#ifndef IMAGE_AGAT140_H
#define IMAGE_AGAT140_H

#include "dsk_tools/disk_image.h"

namespace dsk_tools {
    class imageAgat140: public diskImage
    {
        protected:
        public:
            imageAgat140(Loader * loader);
            virtual int check();
    };
}

#endif // IMAGE_AGAT140_H
