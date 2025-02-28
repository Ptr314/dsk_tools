#ifndef IMAGE_AGAT140_H
#define IMAGE_AGAT140_H

#include "disk_image.h"

namespace dsk_tools {

    // static const int agat_140_logic2raw[16] = {
    //     0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15
    // };

    // static const int agat_140_raw2logic[16] = {
    //     0, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 15
    // };

    class imageAgat140: public diskImage
    {
        protected:

        public:
            imageAgat140(Loader * loader);
            virtual int check() override;
            // virtual int translate_sector_logic2raw(int sector) override;
            // virtual int translate_sector_raw2logic(int sector) override;
    };
}

#endif // IMAGE_AGAT140_H
