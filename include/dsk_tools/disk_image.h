#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include "dsk_tools/definitions.h"
#include "dsk_tools/loader.h"

namespace dsk_tools {

    class diskImage {
        protected:
            std::vector<uint8_t> buffer;
            Loader * loader;
            int format_heads;
            int format_tracks;
            int format_sectors;
            int format_sector_size;
            bool is_loaded;
            bool is_open;
        public:
            diskImage(Loader * loader);
            virtual int check() = 0;    // Checks physical image parameters
            virtual int open() = 0;     // Opens and checks disk's filesystem
            virtual int dir(std::vector<dsk_tools::fileData> * files) = 0;      // List files
            virtual int load();
            virtual int translate_sector(int sector);
            virtual std::byte * get_sector_data(int head, int track, int sector);
    };
}

#endif // DISK_IMAGE_H
