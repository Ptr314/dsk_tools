// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Abstract class for different filesystems

#pragma once

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
        explicit fileSystem(diskImage * image);
        virtual ~fileSystem() = default;

        // Service functions
        [[nodiscard]] virtual Result open() = 0;
        virtual FSCaps get_caps() = 0;
        virtual FS get_fs() const = 0;
        virtual std::string get_delimiter();
        virtual std::vector<std::string> get_save_file_formats() = 0;
        virtual std::vector<std::string> get_add_file_formats() {return {};} ;

        // Disk functions
        virtual std::string information() {return "Not implemented";};
        virtual int get_volume_id() {return volume_id;};
        virtual int translate_sector(int sector) const {return sector;};
        virtual bool get_changed() {return is_changed;};
        virtual void reset_changed() { is_changed = false; };

        // Directories
        virtual Result dir(std::vector<UniversalFile> & files, bool show_deleted) = 0;
        virtual void cd(const UniversalFile & dir, bool & updir) {};
        void cd(const UniversalFile & dir) {bool updir; cd(dir, updir);};
        virtual void cd(const std::string & path, bool & updir) {};
        void cd(const std::string & path) {bool updir; cd(path, updir);};
        virtual void cd_up() {};
        virtual Result mkdir(const std::string & dir_name, UniversalFile & new_dir) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual Result mkdir(const UniversalFile & uf, UniversalFile & new_dir) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual bool is_root() {return true;};

        // Files
        virtual Result find_file(const std::string & file_name, UniversalFile & fd) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual Result get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const = 0;
        virtual Result put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual Result rename_file(const UniversalFile & fd, const std::string & new_name) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual Result delete_file(const UniversalFile & uf) {return Result::error(ErrorCode::NotImplementedYet);};
        virtual std::string file_info(const UniversalFile & fd) {return "";};
        virtual std::vector<ParameterDescription> file_get_metadata(const UniversalFile & fd) {std::vector<ParameterDescription> params; return params;};
        virtual Result file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata) {return Result::error(ErrorCode::NotImplementedYet);};
    };

} // namespace
