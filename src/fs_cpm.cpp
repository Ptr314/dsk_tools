#include <iostream>
#include <fstream>
#include <cstring>
#include <set>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_cpm.h"

namespace dsk_tools {

    fsCPM::fsCPM(diskImage * image):
        fileSystem(image)
    {}

    int fsCPM::get_capabilities()
    {
        return FILE_PROTECTION | FILE_DELETE | FILE_TYPE;
    }

    int fsCPM::open()
    {
        if (!image->get_loaded()) return FDD_OPEN_NOT_LOADED;

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
                .AL1 = 0b00000000,
                .CKS = 16,
                .OFF = 3
            };
        } else
            return FDD_OPEN_BAD_FORMAT;

        is_open = true;
        return FDD_OPEN_OK;
    }

    std::string fsCPM::make_file_name(CPM_DIR_ENTRY & di)
    {
        std::string ext = "";
        for (int i=0; i<3; i++) ext += static_cast<char>(di.E[i] & 0x7F);
        return trim(std::string(reinterpret_cast<char*>(&di.F), 8)) + ((ext.size() > 0)?("."+ext):"");
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

        for (int i = 0; i < catalog_size; i++) {
            uint8_t ST = catalog[i]->ST;
            if (ST != 0xE5 && ST != 0x1F) {
                int extent = catalog[i]->XH*32 + catalog[i]->XL;
                if (extent == 0) {
                    fileData file;
                    file.is_dir = false;
                    file.is_deleted = false;
                    file.name = make_file_name(*catalog[i]);
                    file.size = catalog[i]->RC * 128;

                    file.is_protected = (catalog[i]->E[0] & 0x80) != 0;
                    file.type_str_short = "";
                    file.type_str_short += (catalog[i]->E[1] & 0x80)?"S":""; // System (hidden)
                    file.type_str_short += (catalog[i]->E[2] & 0x80)?"A":""; // Archived


                    std::set<std::string> txts = {".txt", ".doc", ".pas", ".asm"};
                    std::string ext = get_file_ext(file.name);
                    file.preferred_type = (txts.find(ext) != txts.end())?PREFERRED_TEXT:PREFERRED_BINARY;
                    if (ext == ".bas")
                        file.preferred_type = PREFERRED_MBASIC;

                    file.metadata.resize(sizeof(CPM_DIR_ENTRY));
                    std::memcpy(file.metadata.data(), catalog[i], sizeof(CPM_DIR_ENTRY));

                    files->push_back(file);
                } else {
                    fileData * f = &(files->at(files->size()-1));
                    f->size += catalog[i]->RC * 128;
                    f->metadata.resize(f->metadata.size() + sizeof(CPM_DIR_ENTRY));
                    std::memcpy(f->metadata.data() + sizeof(CPM_DIR_ENTRY)*extent, catalog[i], sizeof(CPM_DIR_ENTRY));
                }
            }
        }

        return FDD_OP_OK;
    }

    void fsCPM::cd(const dsk_tools::fileData & dir)
    {}

    std::string fsCPM::file_info(const fileData & fd) {

        std::string result = "";
        std::string attrs = "";

        CPM_DIR_ENTRY dir_entry;
        std::memcpy(&dir_entry, fd.metadata.data(), sizeof(CPM_DIR_ENTRY));

        attrs += (dir_entry.E[0] & 0x80)?"P":"-"; // Read-only
        attrs += (dir_entry.E[1] & 0x80)?"S":"-"; // System (hidden)
        attrs += (dir_entry.E[2] & 0x80)?"A":"-"; // Archived

        result += "{$FILE_NAME}: " +  make_file_name(dir_entry) + "\n";

        int file_size = 0;
        std::string list = "";
        for (int i=0; i < fd.metadata.size() / sizeof(CPM_DIR_ENTRY); i++) {
            list += "{$EXTENT}: " +  std::to_string(i) + "\n";
            std::memcpy(&dir_entry, fd.metadata.data() + i*sizeof(CPM_DIR_ENTRY), sizeof(CPM_DIR_ENTRY));
            file_size += dir_entry.RC*128;
            for (int j=0; j<16; j++) {
                uint8_t AL = dir_entry.AL[j];
                int track = AL / 4 + DPB.OFF;
                int part = AL % 4;
                if (AL != 0 && AL != 0xE5)  {
                    list += "    Block: " + std::to_string(AL);
                    list += " Track: " + std::to_string(track);
                    list += " Part: " + std::to_string(part);
                    list += " Sectors:" ;
                    for (int k=0; k<4; k++) {
                        list += " " + std::to_string(agat_140_cpm2dos[part*4 + k]);
                    }
                    list += "\n";
                }
            }
        }

        result += "{$SIZE}: " +  std::to_string(file_size) + " {$BYTES}\n";
        result += "{$ATTRIBUTES}: " + attrs + " \n";

        result += "\n";
        result += list;

        return result;
    }

    void fsCPM::load_file(const BYTES * dir_records, int extents, BYTES & out)
    {
        out.clear();
        int file_size = 0;
        for (int i=0; i<extents; i++) {
            CPM_DIR_ENTRY dir_entry;
            std::memcpy(&dir_entry, dir_records + i*sizeof(CPM_DIR_ENTRY), sizeof(CPM_DIR_ENTRY));

            file_size += dir_entry.RC*128;

            for (int j=0; j<16; j++) {
                uint8_t AL = dir_entry.AL[j];
                int track = AL / 4 + DPB.OFF;
                int part = AL % 4;
                if (AL != 0 && AL != 0xE5)  {
                    for (int k=0; k<4; k++) {
                        int sector = agat_140_cpm2dos[part*4 + k];
                        auto p = image->get_sector_data(0, track, sector);
                        out.insert(out.end(), p, p + 256);
                    }
                }
            }
        }
        out.resize(file_size);
    }

    BYTES fsCPM::get_file(const fileData & fd)
    {
        BYTES data;
        load_file(reinterpret_cast<const BYTES*>(fd.metadata.data()), fd.metadata.size() / sizeof(CPM_DIR_ENTRY), data);
        return data;
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
