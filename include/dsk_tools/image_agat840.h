#ifndef IMAGE_AGAT840_H
#define IMAGE_AGAT840_H

#include "dsk_tools/image_agat140.h"

namespace dsk_tools {

    class imageAgat840: public imageAgat140
    {
    public:
        imageAgat840(Loader * loader);
        virtual int translate_sector_logic2raw(int sector) override;
        virtual int translate_sector_raw2logic(int sector) override;
    };

}


#endif // IMAGE_AGAT840_H
