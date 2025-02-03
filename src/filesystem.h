#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "disk_image.h"

namespace dsk_tools {

    class fileSystem {
    protected:
        diskImage * image;
        bool is_open;

    public:
        fileSystem(diskImage * image);
        virtual ~fileSystem() {};
        virtual int open() = 0;
        virtual int get_capabilities() = 0;
        virtual std::string get_delimiter();
        virtual int dir(std::vector<dsk_tools::fileData> * files) = 0;
        virtual void cd(const dsk_tools::fileData & dir) = 0;
        virtual BYTES get_file(const fileData & fd) = 0;
    };

} // namespace

#endif // FILESYSTEM_H
