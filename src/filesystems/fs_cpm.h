// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and other definitions for the CP/M filesystem

#ifndef FS_CPM_H
#define FS_CPM_H

#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    // https://stjarnhimlen.se/apple2/Apple.CPM.ref.txt
    struct CPM_DPB
    {
        uint16_t   SPT;     // Total number of sectors per track
        uint8_t    BSH;     // Data allocation block shift factor
        uint8_t    BLM;     // Data allocation block mask
        uint8_t    EXM;     // Extent mask
        uint16_t   DSM;     // Total storage capacity of disk drive in blocks
        uint16_t   DRM;     // Total number of directory entries minus one
        uint8_t    AL0;     // Determines reserved directory blocks
        uint8_t    AL1;     // Determines reserved directory blocks
        uint16_t   CKS;     // Size of directory check vector
        uint16_t   OFF;     // No of reserved tracks at beginning of logical disk

    };

    // https://ciderpress2.com/formatdoc/CPM-notes.html
    struct CPM_DIR_ENTRY
    {
        uint8_t     ST;         // Status
        uint8_t     F[8];       // Name
        uint8_t     E[3];       // Extension
        uint8_t     XL;         // Extent number, low part
        uint8_t     BC;         // Depends on OS
        uint8_t     XH;         // Extent number, high part
        uint8_t     RC;         // Records in the extent
        uint8_t     AL[16];     // Allocation table
    };

    #pragma pack(pop)

    class fsCPM: public fileSystem
    {
    protected:
        CPM_DPB DPB;
        std::string m_filesystem_id;
        std::string make_file_name(CPM_DIR_ENTRY & di);
        void load_file(const BYTES *dir_records, int extents, BYTES & out);

        virtual bool sector_is_free(int head, int track, int sector) override;
        virtual void sector_free(int head, int track, int sector) override;
        virtual bool sector_occupy(int head, int track, int sector) override;
    public:
        fsCPM(diskImage * image, const std::string & filesystem_id);
        virtual int open() override;
        virtual int get_capabilities() override;
        virtual void cd(const dsk_tools::fileData & dir) override;
        virtual void cd_up() override;
        virtual int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) override;
        virtual BYTES get_file(const fileData & fd) override;
        virtual std::string file_info(const fileData & fd) override;
        virtual int file_delete(const fileData & fd) override;
        virtual int file_add(const std::string & file_name, const std::string & format_id) override;
        virtual std::vector<std::string> get_save_file_formats() override;
        virtual std::vector<std::string> get_add_file_formats() override;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
        virtual std::string information() override;
        virtual int translate_sector(int sector) override;
        virtual int mkdir(const std::string & dir_name) override;
        virtual int file_rename(const fileData & fd, const std::string & new_name) override;
        virtual bool is_root() override;
        virtual std::vector<ParameterDescription> file_get_metadata(const fileData & fd) override;
        virtual int file_set_metadata(const fileData & fd, const std::map<std::string, std::string> & metadata) override;
        bool file_find(const std::string & file_name, fileData &fd) override;
    };
}

#endif // FS_CPM_H
