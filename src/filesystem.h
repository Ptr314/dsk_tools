#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "disk_image.h"

namespace dsk_tools {

    class fileSystem {
    protected:
        diskImage * image;
        bool is_open = false;
        int volume_id = -1;

    public:
        fileSystem(diskImage * image);
        virtual ~fileSystem() {};
        virtual int open() = 0;
        virtual int get_capabilities() = 0;
        virtual std::string get_delimiter();
        virtual int dir(std::vector<dsk_tools::fileData> * files) = 0;
        virtual void cd(const dsk_tools::fileData & dir) = 0;
        virtual BYTES get_file(const fileData & fd) = 0;
        virtual std::string file_info(const fileData & fd) = 0;
        virtual std::vector<std::string> get_save_file_formats() = 0;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) = 0;
        virtual std::string information() = 0;
        virtual int get_volume_id() {return volume_id;};
        virtual int translate_sector(int sector) {return sector;};
    };

} // namespace

#endif // FILESYSTEM_H
