#include "filesystem.h"

namespace dsk_tools {
    fileSystem::fileSystem(diskImage * image):
        image(image)
    {}

    std::string fileSystem::get_delimiter()
    {
        return "\\";
    }

    int fileSystem::free_sectors()
    {
        int free_sectors = 0;
        for (int track=0; track < image->get_tracks(); track ++)
            for (int sector=0; sector<image->get_sectors(); sector++)
                free_sectors += sector_is_free(track, sector);
        return free_sectors;
    }

}
