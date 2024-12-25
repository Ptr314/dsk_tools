#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include "dsk_tools/loader.h"

namespace dsk_tools {

    class diskImage {
        protected:
            std::vector<uint8_t> buffer;
            Loader * loader;
        public:
            diskImage(Loader * loader);
            virtual int check() = 0;
            virtual int load();
    };
}

#endif // DISK_IMAGE_H
