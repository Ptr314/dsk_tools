#ifndef FS_DOS33_H
#define FS_DOS33_H

#include "filesystem.h"

namespace dsk_tools {

    #pragma pack(push, 1)

    struct Apple_DOS_VTOC
    {
        uint8_t     _not_used_00;        // 00
        uint8_t     catalog_track;       // 01
        uint8_t     catalog_sector;      // 02
        uint8_t     dos_release;         // 03
        uint8_t     _not_used_04[2];     // 04-05
        uint8_t     volume_id;           // 06
        uint8_t     _not_used_07[0x20];  // 07-26
        uint8_t     pairs_on_sector;     // 27
        uint8_t     _not_used_28[8];     // 28-2F
        uint8_t     last_track;          // 30
        uint8_t     direction;           // 31
        uint8_t     _not_used_32[2];     // 32-33
        uint8_t     tracks_total;        // 34
        uint8_t     sectors_on_track;    // 35
        uint16_t    bytes_per_sector;    // 36-37
        uint32_t    free_sectors[50];    // 38-FF
    };

    struct Agat_VTOC
    {
        uint8_t     _not_used_00;        // 00
        uint8_t     catalog_track;       // 01
        uint8_t     catalog_sector;      // 02
        uint8_t     dos_release;         // 03
        uint8_t     _not_used_04[2];     // 04-05
        uint8_t     volume_id;           // 06
        uint8_t     _not_used_07;        // 07
        uint8_t     volume_name[31];     // 08-26
        uint8_t     pairs_on_sector;     // 27
        uint8_t     _not_used_28[8];     // 28-2F
        uint8_t     last_track;          // 30
        uint8_t     direction;           // 31
        uint8_t     _not_used_32[2];     // 32-33
        uint8_t     tracks_total;        // 34
        uint8_t     sectors_on_track;    // 35
        uint16_t    bytes_per_sector;    // 36-37
        uint32_t    free_sectors[50];    // 38-FF
    };

    struct Apple_DOS_File
    {
        uint8_t     tbl_track;      // 00
        uint8_t     tbl_sector;     // 01
        uint8_t     type;           // 02
        uint8_t     name[30];       // 03-20
        uint16_t    size;           // 21-22
    };

    struct Apple_DOS_Catalog
    {
        uint8_t         _not_used_00;       // 00
        uint8_t         next_track;         // 01
        uint8_t         next_sector;        // 02
        uint8_t         _not_used_03[8];    // 03-0A
        Apple_DOS_File  files[7];           // 0B-FF
    };

    struct Apple_DOS_TS_List
    {
        uint8_t         _not_used_00;       // 00
        uint8_t         next_track;         // 01
        uint8_t         next_sector;        // 02
        uint8_t         _not_used_03[2];    // 03-04
        uint16_t        offset;             // 05-06
        uint8_t         _not_used_07[5];    // 07-0B
        uint8_t         ts[122][2];         // 0C-FF
    };

    #pragma pack(pop)


    class fsDOS33: public fileSystem
    {
    protected:
        dsk_tools::Apple_DOS_VTOC * VTOC;
    public:
        fsDOS33(diskImage * image);
        virtual int open() override;
        virtual int get_capabilities() override;
        virtual void cd(const dsk_tools::fileData & dir) override;
        virtual int dir(std::vector<dsk_tools::fileData> * files) override;
        virtual BYTES get_file(const fileData & fd) override;
        virtual std::string file_info(const fileData & fd) override;
    };
}

#endif // FS_DOS33_H
