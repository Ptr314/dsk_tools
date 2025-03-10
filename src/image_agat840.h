#ifndef IMAGE_AGAT840_H
#define IMAGE_AGAT840_H

#include "image_agat140.h"

namespace dsk_tools {

    class imageAgat840: public imageAgat140
    {
    public:
        imageAgat840(Loader * loader);
        virtual uint8_t *get_sector_data(int head, int track, int sector) override;
    };

}


#endif // IMAGE_AGAT840_H
