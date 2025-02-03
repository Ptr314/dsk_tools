#include <algorithm>
#include <cstring>
#include <filesystem>
#include <set>

#include "utils.h"
#include "fs_spriteos.h"

namespace dsk_tools {

    fsSpriteOS::fsSpriteOS(diskImage * image):
        fileSystem(image)
    {}

    int fsSpriteOS::get_capabilities()
    {
        return FILE_PROTECTION | FILE_DELETE | FILE_DIRS;
    }

    int fsSpriteOS::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        memcpy(&CURRENT_DIR, image->get_sector_data(0, 0, 0), sizeof(CURRENT_DIR));
        memcpy(&DPB, &CURRENT_DIR, sizeof(DPB));

        memset(&CURRENT_DIR.NAME, 0xA0, 15);
        current_path.push_back(CURRENT_DIR);

        is_open = true;

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

    int fsSpriteOS::dir(std::vector<dsk_tools::fileData> * files)
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
            if (dir_entry->NAME[0] != 0) {
                fileData file;
                file.name = trim(koi7_to_utf(dir_entry->NAME, 15));
                file.size = dir_entry->FILELEN[0] + (dir_entry->FILELEN[1] << 8) + (dir_entry->FILELEN[2] << 16);
                file.is_dir = (dir_entry->STATUS & 0x01) != 0;
                file.is_deleted = dir_entry->NAME[0] == 0xFF;

                std::set<std::string> txts = {".txt", ".doc", ".pas", ".cmd", ".def", ".hlp"};
                namespace fs = std::filesystem;
                std::string ext = fs::path(file.name).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
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

    void fsSpriteOS::cd(const dsk_tools::fileData & dir)
    {
        if (dir.name == "..") {
            current_path.pop_back();
            CURRENT_DIR = current_path.back();
        } else {
            memcpy(&CURRENT_DIR, dir.metadata.data(), sizeof(CURRENT_DIR));
            current_path.push_back(CURRENT_DIR);
        }
    }

}
