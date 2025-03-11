#ifndef IMAGE_AGAT140_H
#define IMAGE_AGAT140_H

#include "disk_image.h"

namespace dsk_tools {

    class imageAgat140: public diskImage
    {
        protected:

        public:
            imageAgat140(Loader * loader);
            virtual int check() override;
    };
}

#endif // IMAGE_AGAT140_H
