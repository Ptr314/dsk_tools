#include <iostream>
#include <stdexcept>
#include <cstring>
#include <set>
#include <fstream>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_spriteos.h"

namespace dsk_tools {

    fsSpriteOS::fsSpriteOS(diskImage * image):
        fileSystem(image)
    {}

    int fsSpriteOS::get_capabilities()
    {
        return FILE_PROTECTION | FILE_DIRS;
    }

    int fsSpriteOS::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        memcpy(&CURRENT_DIR, image->get_sector_data(0, 0, 0), sizeof(CURRENT_DIR));
        memcpy(&DPB, &CURRENT_DIR, sizeof(DPB));

        memset(&CURRENT_DIR.NAME, 0xA0, 15);
        current_path.push_back(CURRENT_DIR);

        is_open = true;

        volume_id = DPB.VOLUME;

        return FDD_OPEN_OK;
    }

    void fsSpriteOS::load_file(const SPRITE_OS_DIR_ENTRY & dir_entry, BYTES & out)
    {
        std::vector<uint8_t> buffer;
        int tr;
        uint8_t * p;

        if (dir_entry.LEVEL == 1) {
            buffer.resize(256);
            tr = dir_entry.INFADR / image->get_sectors();
            p = image->get_sector_data(0, tr, dir_entry.INFADR % image->get_sectors());
            memcpy(buffer.data(), p, 256);
        } else
        if (dir_entry.LEVEL == 2) {
            uint16_t BLOCKS_LIST[128];
            tr = dir_entry.INFADR / image->get_sectors();
            p = image->get_sector_data(0, tr, dir_entry.INFADR % image->get_sectors());
            memcpy(BLOCKS_LIST, p, 256);

            int i = 0;
            std::vector<uint8_t> block;
            block.resize(256);
            while (BLOCKS_LIST[i] != 0 && i<256) {
                tr = BLOCKS_LIST[i] / image->get_sectors();
                p = image->get_sector_data(0, tr, BLOCKS_LIST[i] % image->get_sectors());

                memcpy(block.data(), p, 256);
                buffer.insert(buffer.end(), block.begin(), block.end());
                i++;
            }
        } else
            throw std::runtime_error("Unknown DIR_ENTRY.LEVEL value: " + std::to_string(dir_entry.LEVEL));

        int expected_size = dir_entry.FILELEN[0] + (dir_entry.FILELEN[1]<<8) + (dir_entry.FILELEN[2]<<16);
        if (expected_size > buffer.size()) {
            throw std::runtime_error("File is shorter than expected");
        } else {
            buffer.resize(expected_size);
        }

        out = buffer;
    }

    int fsSpriteOS::dir(std::vector<dsk_tools::fileData> * files, bool show_deleted)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        if (current_path.size() > 1) {
            fileData updir;
            updir.name = "..";
            updir.is_dir = true;
            updir.is_deleted = false;
            files->push_back(updir);
        }

        BYTES buffer;
        load_file(CURRENT_DIR, buffer);

        for (int i=0; i < buffer.size() / sizeof(SPRITE_OS_DIR_ENTRY); i++) {
            SPRITE_OS_DIR_ENTRY * dir_entry = reinterpret_cast<SPRITE_OS_DIR_ENTRY*>(buffer.data() + i*sizeof(SPRITE_OS_DIR_ENTRY));
            bool is_deleted = dir_entry->NAME[0] == 0xFF;
            if (dir_entry->NAME[0] != 0 && (!is_deleted || show_deleted)) {
                fileData file;
                file.name = trim(agat_to_utf(dir_entry->NAME, 15));
                file.size = dir_entry->FILELEN[0] + (dir_entry->FILELEN[1] << 8) + (dir_entry->FILELEN[2] << 16);
                file.is_dir = (dir_entry->STATUS & 0x01) != 0;
                file.is_deleted = is_deleted;

                std::set<std::string> txts = {".txt", ".doc", ".pas", ".cmd", ".def", ".hlp", ".gid"};
                std::string ext = get_file_ext(file.name);
                file.preferred_type = (txts.find(ext) != txts.end())?PREFERRED_TEXT:PREFERRED_BINARY;

                file.metadata.resize(sizeof(SPRITE_OS_DIR_ENTRY));
                memcpy(file.metadata.data(), dir_entry, sizeof(SPRITE_OS_DIR_ENTRY));

                files->push_back(file);
            }
        }

        return FDD_OP_OK;
    }

    BYTES fsSpriteOS::get_file(const fileData & fd)
    {
        BYTES data;

        const SPRITE_OS_DIR_ENTRY * dir_entry = reinterpret_cast<const SPRITE_OS_DIR_ENTRY*>(fd.metadata.data());

        load_file(*dir_entry, data);

        return data;
    }

    void fsSpriteOS::cd_up()
    {
        if (current_path.size() > 1) {
            current_path.pop_back();
            CURRENT_DIR = current_path.back();
        }
    }

    void fsSpriteOS::cd(const dsk_tools::fileData & dir)
    {
        if (dir.name == "..") {
            cd_up();
        } else {
            memcpy(&CURRENT_DIR, dir.metadata.data(), sizeof(CURRENT_DIR));
            current_path.push_back(CURRENT_DIR);
        }
    }

    std::string fsSpriteOS::file_info(const fileData & fd) {
        std::string result = "";
        std::string attrs = "";

        const SPRITE_OS_DIR_ENTRY * dir_entry = reinterpret_cast<const SPRITE_OS_DIR_ENTRY*>(fd.metadata.data());

        attrs += (dir_entry->STATUS & 0x80)?"P":"-";
        attrs += (dir_entry->STATUS & 0x40)?"B":"-";
        attrs += (dir_entry->STATUS & 0x20)?"U":"-";
        attrs += (dir_entry->STATUS & 0x10)?"U":"-";
        attrs += (dir_entry->STATUS & 0x08)?"T":"-";
        attrs += (dir_entry->STATUS & 0x04)?"S":"-";
        attrs += (dir_entry->STATUS & 0x02)?"H":"-";
        attrs += (dir_entry->STATUS & 0x01)?"D":"-";

        int year_bcd = dir_entry->DATE >> 11;
        int year = (year_bcd & 0xF) + (year_bcd >> 4) * 10 + 1980;
        std::string month = dsk_tools::toBCD((dir_entry->DATE >> 6) & 0b11111);
        std::string day = dsk_tools::toBCD(dir_entry->DATE & 0b111111);

        std::string date_str = day + "-" + month + "-" + std::to_string(year);

        result += "{$FILE_NAME}: " +  trim(agat_to_utf(dir_entry->NAME, 15)) + "\n";
        result += "{$SIZE}: " +  std::to_string(dir_entry->FILELEN[0] + (dir_entry->FILELEN[1] << 8) + (dir_entry->FILELEN[2] << 16)) + " {$BYTES}\n";
        result += "{$ATTRIBUTES}: $" + dsk_tools::int_to_hex(dir_entry->STATUS) + " (" + attrs + ") \n";
        result += "{$DATE}: " + date_str + " ($" + dsk_tools::int_to_hex(dir_entry->DATE) +")\n";
        result += "USRINF: " + dsk_tools::toHexList(dir_entry->USRINF, sizeof(dir_entry->USRINF), "$") + "\n";
        return result;
    }

    std::vector<std::string> fsSpriteOS::get_save_file_formats()
    {
        return {"FILE_SOS", "FILE_BINARY"};
    }

    std::vector<std::string> fsSpriteOS::get_add_file_formats()
    {
        return {"FILE_SOS", "FILE_BINARY"};
    }

    int fsSpriteOS::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
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
            if (format_id == "FILE_SOS") {
                std::cout << dsk_tools::base64_encode(buffer, 32) << std::endl;
            }
            return FDD_WRITE_OK;
        } else {
            return FDD_WRITE_ERROR_READING;
        }
    }

    std::string fsSpriteOS::information()
    {
        return agat_sos_info(DPB);
    }

    bool fsSpriteOS::sector_is_free(int head, int track, int sector)
    {
        return false;
    }

    void fsSpriteOS::sector_free(int head, int track, int sector)
    {

    }

    bool fsSpriteOS::sector_occupy(int head, int track, int sector)
    {
        return false;
    }

    bool fsSpriteOS::file_delete(const fileData & fd)
    {
        return false;
    }

    int fsSpriteOS::file_add(const std::string & file_name, const std::string & format_id)
    {
        return false;
    }

    int fsSpriteOS::mkdir(const std::string & dir_name)
    {
        return FDD_DIR_ERROR;
    }

    int fsSpriteOS::file_rename(const fileData & fd, const std::string & new_name)
    {
        return FILE_RENAME_OK;
    }

    bool fsSpriteOS::is_root()
    {
        return current_path.size() == 1;
    }

    std::vector<ParameterDescription> fsSpriteOS::file_get_metadata(const fileData &fd)
    {
        std::vector<ParameterDescription> params = {};

        return params;
    }

    int fsSpriteOS::file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata)
    {
        return FILE_METADATA_OK;
    }


}
