#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "disk_image.h"

namespace dsk_tools {

    class fileSystem {
    protected:
        diskImage * image;
        bool is_open = false;
        int volume_id = -1;
        bool is_changed = false;

        virtual bool sector_is_free(int head, int track, int sector) = 0;
        virtual void sector_free(int head, int track, int sector) = 0;
        virtual bool sector_occupy(int head, int track, int sector) = 0;
        virtual int free_sectors() {return 0;};

    public:
        fileSystem(diskImage * image);
        virtual ~fileSystem() {};
        virtual int open() = 0;
        virtual int get_capabilities() = 0;
        virtual std::string get_delimiter();
        virtual int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) = 0;
        virtual void cd(const dsk_tools::fileData & dir) = 0;
        virtual void cd_up() = 0;
        virtual BYTES get_file(const fileData & fd) = 0;
        virtual std::string file_info(const fileData & fd) = 0;
        virtual bool file_delete(const fileData & fd) = 0;
        virtual int file_add(const std::string & file_name, const std::string & format_id) = 0;
        virtual std::vector<std::string> get_save_file_formats() = 0;
        virtual std::vector<std::string> get_add_file_formats() = 0;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) = 0;
        virtual std::string information() = 0;
        virtual int get_volume_id() {return volume_id;};
        virtual int translate_sector(int sector) {return sector;};
        virtual bool get_changed() {return is_changed;};
    };

} // namespace

#endif // FILESYSTEM_H
