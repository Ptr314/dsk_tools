#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_fil.h"

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

        // file.is_dir = catalog->files[i].type == 0xFF;
        // file.is_deleted = is_deleted;
        // file.is_protected = (catalog->files[i].type & 0x80) != 0;
        // file.attributes = catalog->files[i].type & 0x7F;
        // memcpy(&file.original_name, &catalog->files[i].name, 30);
        // file.original_name_length = 30;

        // auto T = agat_attr_to_type(catalog->files[i].type);
        // file.type_str_short = std::string(agat_file_types[T]);

        // if (T == 0)
        //     file.preferred_type = PREFERRED_TEXT;
        // else if (T == 2)
        //     file.preferred_type = PREFERRED_AGATBASIC;
        // else
        //     file.preferred_type = PREFERRED_BINARY;

        // file.size = catalog->files[i].size * 256;

        // files->push_back(file);

        return FDD_OP_OK;
    }

    BYTES fsFIL::get_file(const fileData & fd)
    {
        BYTES data;

        // TODO: implement

        return data;
    }

    std::string fsFIL::file_info(const fileData & fd) {

        std::string result = "";

        return result;
    }

    std::vector<std::string> fsFIL::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    int fsFIL::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
    {

        BYTES buffer = get_file(fd);
        if (buffer.size() > 0) {
            if (format_id == "FILE_BINARY") {
                std::ofstream file(file_name, std::ios::binary);

                if (!file.good()) {
                    return FDD_WRITE_ERROR;
                }

                file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
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
        // TODO: Implement
        is_changed = true;
        return FILE_RENAME_OK;
    }

    std::vector<ParameterDescription> fsFIL::file_get_metadata(const fileData &fd)
    {
        std::vector<ParameterDescription> params;
        params.push_back({"filename", "{$META_FILENAME}", ParamType::String, fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, fd.is_protected?"true":"false"});

        // std::vector<std::pair<std::string, std::string>> options;
        // for (int i = 0; i < agat_file_types.size(); i++) {
        //     options.emplace_back(agat_file_types[i], std::to_string(i));
        // }

        // auto T = attr_to_type(fd.attributes);
        // params.push_back({"type", "{$META_TYPE}", ParamType::Enum, std::to_string(T), options});


        // const dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<const dsk_tools::Apple_DOS_File *>(fd.metadata.data());
        // int list_track = dir_entry->tbl_track;
        // int list_sector = dir_entry->tbl_sector;

        // Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));
        // BYTES ts_custom(fd.is_dir?8:9);
        // void * from = &(ts_list->_not_used_03);
        // std::memcpy(ts_custom.data(), from, ts_custom.size());
        // for (int i = 0; i < ts_custom.size(); i++) {
        //     params.push_back({"extended_"+std::to_string(i), "{$META_EXTENDED} #"+std::to_string(i), ParamType::Byte,std::to_string(ts_custom[i])});
        // }

        return params;
    }

    int fsFIL::file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata)
    {
        // uint8_t new_type = 0;
        // bool is_protected = false;
        // BYTES ts_custom(fd.is_dir?8:9);

        // const std::string ext_prefix = "extended_";

        // for (const auto& pair : metadata) {
        //     const std::string& key = pair.first;
        //     const std::string& val = pair.second;
        //     std::cout << "+ " << key << "=" << val << std::endl;
        //     if (key == "filename") {
        //         if (val != fd.name) {
        //             int res = file_rename(fd, val);
        //             if (res != FILE_RENAME_OK)
        //                 return FILE_METADATA_ERROR;
        //         }
        //     } else
        //         if (key == "type") {
        //             uint8_t val_i = std::stoi(val);
        //             if (val_i == 0) new_type = 0;
        //             else
        //                 if (val_i == 7) new_type = 0xFF;
        //                 else
        //                     new_type = 0x01 << (val_i - 1);
        //         } else
        //             if (key == "protected") {
        //                 is_protected = (val == "true");
        //             } else
        //                 if (key.compare(0, ext_prefix.size(), ext_prefix) == 0) {
        //                     // std::cout << "! " << ext_prefix.size() << " : " << key << std::endl;
        //                     std::string number_part = key.substr(ext_prefix.size());
        //                     int number_out = std::stoi(number_part);

        //                     uint8_t val_i = std::stoi(val);
        //                     ts_custom[number_out] = val_i;
        //                 }

        //     // std::cout << "=== " << key.compare(0, ext_prefix.size(), ext_prefix) << std::endl;
        // }

        // if (is_protected) new_type |= 0x80;
        // Apple_DOS_Catalog * catalog = reinterpret_cast<dsk_tools::Apple_DOS_Catalog *>(image->get_sector_data(0, fd.position[0], fd.position[1]));
        // dsk_tools::Apple_DOS_File * dir_entry = reinterpret_cast<dsk_tools::Apple_DOS_File *>(&(catalog->files[fd.position[2]]));

        // dir_entry->type = new_type;

        // int list_track = dir_entry->tbl_track;
        // int list_sector = dir_entry->tbl_sector;

        // Apple_DOS_TS_List * ts_list = reinterpret_cast<dsk_tools::Apple_DOS_TS_List *>(image->get_sector_data(0, list_track, list_sector));

        // void * copy_to = &(ts_list->_not_used_03);
        // std::memcpy(copy_to, ts_custom.data(), ts_custom.size());

        is_changed = true;
        return FILE_METADATA_OK;
    }

}
