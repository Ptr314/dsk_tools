// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for different filesystems

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "disk_image.h"
#include <map>

namespace dsk_tools {

    enum class ParamType {
        String,
        Byte,
        Word,
        DWord,
        Enum,
        Checkbox
    };

    struct ParameterDescription {
        std::string id;
        std::string name;
        ParamType type;
        std::string initialValue;

        std::vector<std::pair<std::string, std::string>> enumOptions; // Только для Enum
    };

    class fileSystem {
    protected:
        diskImage * image;
        bool is_open = false;
        int volume_id = -1;
        bool is_changed = false;

        virtual bool sector_is_free(int head, int track, int sector) { return false;};
        virtual void sector_free(int head, int track, int sector) {};
        virtual bool sector_occupy(int head, int track, int sector) {return false;};
        virtual int free_sectors() {return 0;};

    public:
        fileSystem(diskImage * image);
        virtual ~fileSystem() {};

        // Service functions
        virtual int open() = 0;
        virtual FSCaps getCaps() = 0;
        virtual FS getFS() const = 0;
        virtual std::string get_delimiter();
        virtual std::vector<std::string> get_save_file_formats() = 0;
        virtual std::vector<std::string> get_add_file_formats() {return {};} ;

        // Disk functions
        virtual std::string information() = 0;
        virtual int get_volume_id() {return volume_id;};
        virtual int translate_sector(int sector) const {return sector;};
        virtual bool get_changed() {return is_changed;};

        // Directories
        virtual Result dir(std::vector<UniversalFile> & files, bool show_deleted) = 0;
            virtual void cd(const dsk_tools::fileData & dir) {};
        virtual void cd(const UniversalFile & dir) {};
        virtual void cd(const std::string & path) {};
        virtual void cd_up() {};
        virtual Result mkdir(const std::string & dir_name) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual bool is_root() {return true;};

        // Files
            virtual BYTES get_file(const fileData & fd) = 0;
        virtual Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const = 0;
        virtual Result put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace) = 0;
        virtual Result delete_file(const UniversalFile & uf) = 0;
        virtual std::string file_info(const fileData & fd) {return "";};
        virtual std::string file_info(const UniversalFile & fd) {return "";};
        virtual int file_delete(const fileData & fd) {return FILE_DELETE_ERROR;};
        virtual int file_add(const std::string & file_name, const std::string & format_id) {return FILE_ADD_ERROR;};
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) = 0;
            virtual int file_rename(const fileData & fd, const std::string & new_name) {return FILE_RENAME_ERROR;};
        virtual Result file_rename(const UniversalFile & fd, const std::string & new_name) {return Result::error(ErrorCode::FileRenameError);};
            virtual std::vector<dsk_tools::ParameterDescription> file_get_metadata(const fileData & fd) = 0;
        virtual std::vector<ParameterDescription> file_get_metadata(const UniversalFile & fd) = 0;
            virtual int file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata) = 0;
        virtual Result file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata) = 0;
            virtual bool file_find(const std::string & file_name, fileData & fd) = 0;
        // virtual bool file_find(const std::string & file_name, UniversalFile & fd) = 0;
    };

} // namespace

#endif // FILESYSTEM_H
