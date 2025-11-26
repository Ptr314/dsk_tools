// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class which represents a .FIL as a single-file contaiter - filesystem part

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_fil.h"
#include "fs_dos33.h"

namespace dsk_tools {

    fsFIL::fsFIL(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsFIL::getCaps()
    {
        return FSCaps::Protect | FSCaps::Types | FSCaps::Rename;
    }

    int fsFIL::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        is_open = true;

        return FDD_OPEN_OK;
    }

    std::string fsFIL::file_info(const UniversalFile & fd) {

        std::string result;

        auto * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        const auto size = image->get_size() - sizeof(FIL_header);

        result += "{$FILE_NAME}: " +  trim(agat_to_utf(header->name, 30)) + " (" + toHexList(header->name, 30, "$") +")\n";
        result += "{$SIZE}: " + std::to_string(size) + " {$BYTES} (" + std::to_string(size/256) + " {$SECTORS})\n";
        result += "    {$TYPE}: " + std::string(agat_file_types[agat_attr_to_type(header->type)]) + " ($" + int_to_hex(header->type) + ")\n";
        result += "    {$PROTECTED}: " + (((header->type & 0x80) > 0)?std::string("{$YES}"):std::string("{$NO}")) + "\n";

        BYTES ts_custom(9);
        const void * from = &(header->tsl);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        result += "    {$CUSTOM_DATA}: [" +  toHexList(ts_custom, "$") +"]\n";

        result += "\n";
        BYTES buffer;
        auto res = get_file(fd, "", buffer);
        result += agat_vr_info(buffer);

        return result;
    }

    std::vector<std::string> fsFIL::get_save_file_formats()
    {
        return {"FILE_FIL", "FILE_BINARY"};
    }

    Result fsFIL::rename_file(const UniversalFile &fd, const std::string &new_name)
    {
        auto * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        const BYTES name_str = utf_to_agat(new_name);
        const auto len = name_str.size();
        std::memset(header->name, 0xA0, sizeof(header->name));
        std::memcpy(header->name, name_str.data(), (len <= sizeof(header->name))?len:sizeof(header->name));

        is_changed = true;
        return Result::ok();
    }

    Result fsFIL::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        data.clear();
        auto * raw_data = image->get_sector_data(0,0,0);
        data.insert(data.end(), raw_data + sizeof(FIL_header), raw_data + image->get_size());
        return Result::ok();
    }

    Result fsFIL::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const auto * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        UniversalFile f;

        f.name = trim(agat_to_utf(header->name, 30));
        f.size = image->get_size()-40;
        f.is_dir = header->type == 0xFF;
        f.is_deleted = false;
        f.is_protected = (header->type & 0x80) != 0;
        f.attributes = header->type & 0x7F;
        f.original_name.resize(30);
        memcpy(f.original_name.data(), header->name, f.original_name.size());
        auto T = agat_attr_to_type(header->type);
        f.type_label = std::string(agat_file_types[T]);
        f.type_preferred = agat_preferred_file_type_new(T);

        files.push_back(f);

        return Result::ok();
    }

    std::vector<ParameterDescription> fsFIL::file_get_metadata(const UniversalFile & fd)
    {
        const auto * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        std::vector<std::pair<std::string, std::string>> options;
        options.reserve(agat_file_types.size());
        for (int i = 0; i < agat_file_types.size(); i++) {
            options.emplace_back(agat_file_types[i], std::to_string(i));
        }

        const auto T = agat_attr_to_type(header->type & 0x7F);
        params.push_back({"type", "{$META_TYPE}", ParamType::Enum, std::to_string(T), options});

        BYTES ts_custom(9);
        const void * from = &(header->tsl);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        for (int i = 0; i < ts_custom.size(); i++) {
            params.push_back({"extended_"+std::to_string(i), "{$META_EXTENDED} #"+std::to_string(i), ParamType::Byte,std::to_string(ts_custom[i])});
        }

        return params;
    }

    Result fsFIL::file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata)
    {
        auto * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        uint8_t new_type = 0;
        bool is_protected = false;
        BYTES ts_custom(9);

        const std::string ext_prefix = "extended_";

        for (const auto& pair : metadata) {
            const std::string& key = pair.first;
            const std::string& val = pair.second;
            std::cout << "+ " << key << "=" << val << std::endl;
            if (key == "filename") {
                if (val != fd.name) {
                    if (!rename_file(fd, val)) return Result::error(ErrorCode::FileRenameError);
                }
            } else
            if (key == "type") {
                const uint8_t val_i = std::stoi(val);
                if (val_i == 0) new_type = 0;
                else
                    if (val_i == 7) new_type = 0xFF;
                    else
                        new_type = 0x01 << (val_i - 1);
            } else
            if (key == "protected") {
                is_protected = (val == "true");
            } else
            if (key.compare(0, ext_prefix.size(), ext_prefix) == 0) {
                std::string number_part = key.substr(ext_prefix.size());
                const int number_out = std::stoi(number_part);

                const uint8_t val_i = std::stoi(val);
                ts_custom[number_out] = val_i;
            }
        }

        if (is_protected) new_type |= 0x80;
        header->type = new_type;
        std::memcpy(header->tsl, ts_custom.data(), ts_custom.size());

        is_changed = true;
        return Result::ok();
    }

}
