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

    int fsFIL::get_capabilities()
    {
        return FILE_PROTECTION | FILE_TYPE | FILE_RENAME;
    }

    int fsFIL::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        is_open = true;

        return FDD_OPEN_OK;
    }

    int fsFIL::dir(std::vector<dsk_tools::fileData> * files, bool show_deleted)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        fileData file;

        FIL_header * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        file.is_dir = header->type == 0xFF;
        file.is_deleted = false;
        file.is_protected = (header->type & 0x80) != 0;
        file.attributes = header->type & 0x7F;
        memcpy(&file.original_name, header->name, 30);
        file.original_name_length = 30;

        file.name = trim(agat_to_utf(header->name, 30));

        auto T = agat_attr_to_type(header->type);
        file.type_str_short = std::string(agat_file_types[T]);

        file.preferred_type = agat_preferred_file_type(T);

        file.size = image->get_size()-40;

        files->push_back(file);

        return FDD_OP_OK;
    }

    BYTES fsFIL::get_file(const fileData & fd)
    {
        BYTES data;

        uint8_t * raw_data = reinterpret_cast<uint8_t *>(image->get_sector_data(0,0,0));

        data.insert(data.end(), raw_data + sizeof(FIL_header), raw_data + image->get_size());

        return data;
    }

    std::string fsFIL::file_info(const fileData & fd) {

        std::string result = "";

        FIL_header * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        int size = image->get_size() - sizeof(FIL_header);

        result += "{$FILE_NAME}: " +  trim(agat_to_utf(header->name, 30)) + " (" + toHexList(header->name, 30, "$") +")\n";
        result += "{$SIZE}: " + std::to_string(size) + " {$BYTES} (" + std::to_string(size/256) + " {$SECTORS})\n";
        result += "    {$TYPE}: " + std::string(agat_file_types[agat_attr_to_type(header->type)]) + " ($" + int_to_hex(header->type) + ")\n";
        result += "    {$PROTECTED}: " + (((header->type & 0x80) > 0)?std::string("{$YES}"):std::string("{$NO}")) + "\n";

        BYTES ts_custom(9);
        void * from = &(header->tsl);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        result += "    {$CUSTOM_DATA}: [" +  toHexList(ts_custom, "$") +"]\n";

        return result;
    }

    std::vector<std::string> fsFIL::get_save_file_formats()
    {
        return {"FILE_FIL", "FILE_BINARY"};
    }

    int fsFIL::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
    {

        if (image->get_size() > sizeof(FIL_header)) {
            if (format_id == "FILE_BINARY") {
                BYTES buffer = get_file(fd);
                std::ofstream file(file_name, std::ios::binary);
                if (!file.good()) return FDD_WRITE_ERROR;
                file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            } else
            if (format_id == "FILE_FIL") {
                std::ofstream file(file_name, std::ios::binary);
                if (!file.good()) return FDD_WRITE_ERROR;
                file.write(reinterpret_cast<char*>(image->get_sector_data(0,0,0)), image->get_size());
            } else
                return FDD_WRITE_UNSUPPORTED;

            return FDD_WRITE_OK;
        } else {
            return FDD_WRITE_ERROR_READING;
        }
    }

    std::string fsFIL::information()
    {
        std::string result;

        return result;
    }


    int fsFIL::file_rename(const fileData & fd, const std::string & new_name)
    {
        FIL_header * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        BYTES name_str = utf_to_agat(new_name);
        int len = name_str.size();
        std::memset(header->name, 0xA0, sizeof(header->name));
        std::memcpy(header->name, name_str.data(), (len <= sizeof(header->name))?len:sizeof(header->name));

        is_changed = true;
        return FILE_RENAME_OK;
    }

    std::vector<ParameterDescription> fsFIL::file_get_metadata(const fileData &fd)
    {
        FIL_header * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        std::vector<std::pair<std::string, std::string>> options;
        for (int i = 0; i < agat_file_types.size(); i++) {
            options.emplace_back(agat_file_types[i], std::to_string(i));
        }

        auto T = agat_attr_to_type(header->type & 0x7F);
        params.push_back({"type", "{$META_TYPE}", ParamType::Enum, std::to_string(T), options});


        BYTES ts_custom(9);
        void * from = &(header->tsl);
        std::memcpy(ts_custom.data(), from, ts_custom.size());
        for (int i = 0; i < ts_custom.size(); i++) {
            params.push_back({"extended_"+std::to_string(i), "{$META_EXTENDED} #"+std::to_string(i), ParamType::Byte,std::to_string(ts_custom[i])});
        }

        return params;
    }

    int fsFIL::file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata)
    {
        FIL_header * header = reinterpret_cast<FIL_header *>(image->get_sector_data(0,0,0));

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
                    int res = file_rename(fd, val);
                    if (res != FILE_RENAME_OK)
                        return FILE_METADATA_ERROR;
                }
            } else
            if (key == "type") {
                uint8_t val_i = std::stoi(val);
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
                    int number_out = std::stoi(number_part);

                    uint8_t val_i = std::stoi(val);
                    ts_custom[number_out] = val_i;
                }
        }

        if (is_protected) new_type |= 0x80;
        header->type = new_type;
        std::memcpy(header->tsl, ts_custom.data(), ts_custom.size());

        is_changed = true;
        return FILE_METADATA_OK;
    }

    bool fsFIL::file_find(const std::string & file_name, fileData & fd)
    {
        return false;
    }


}
