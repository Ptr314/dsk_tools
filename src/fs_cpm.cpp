#include <iostream>
#include <fstream>
#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_cpm.h"

namespace dsk_tools {

    fsCPM::fsCPM(diskImage * image):
        fileSystem(image)
    {}

    int fsCPM::get_capabilities()
    {
        return FILE_PROTECTION | FILE_DELETE;
    }

    int fsCPM::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

        std::cout << image->get_type_id() << std::endl;
        if (image->get_type_id() == "TYPE_AGAT_140") {
            // n = 10, BLS = 1024 (2**n)
            DPB = {
                .SPT = 32,          // 256*16/128
                .BSH = 3,           // n-7
                .BLM = 7,           // 2**BSH - 1
                .EXM = 0,           // 2**(BHS-2) - 1 if DSM<256
                .DSM = 127,
                .DRM = 63,
                .AL0 = 0b11000000,
                .AL1 = 0,
                .CKS = 16,
                .OFF = 3
            };
        } else
            return FDD_OPEN_BAD_FORMAT;

        is_open = true;
        // volume_id = VTOC->volume_id;
        return FDD_OPEN_OK;
    }

    int fsCPM::dir(std::vector<dsk_tools::fileData> * files)
    {
        if (!is_open) return FDD_OP_NOT_OPEN;

        files->clear();

        int directory_sectors = 6;
        int entries_in_sector = 8;
        int catalog_size = directory_sectors * entries_in_sector;

        CPM_DIR_ENTRY * catalog[catalog_size];

        for (int i = 0; i < directory_sectors; i++) {
            uint8_t * sector = image->get_sector_data(0, DPB.OFF, agat_140_cpm2dos[i]);
            for (int j = 0; j < entries_in_sector; j++)
                catalog[i*entries_in_sector + j] = reinterpret_cast<CPM_DIR_ENTRY*>(sector + j*sizeof(CPM_DIR_ENTRY));
        }

        return FDD_OP_OK;
    }

    BYTES fsCPM::get_file(const fileData & fd)
    {
        BYTES data;

        // TODO: get file here

        return data;
    }

    void fsCPM::cd(const dsk_tools::fileData & dir)
    {}

    std::string fsCPM::file_info(const fileData & fd) {

        std::string result = "";

        // TODO: file info here

        return result;
    }

    std::vector<std::string> fsCPM::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    int fsCPM::save_file(const std::string & format_id, const std::string & file_name, const fileData &fd)
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

    std::string fsCPM::information()
    {
        return "";
    }


}
