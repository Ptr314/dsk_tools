#include "filesystem.h"

namespace dsk_tools {
    fileSystem::fileSystem(diskImage * image):
        image(image)
    {}

    std::string fileSystem::get_delimiter()
    {
        return "\\";
    }
}
