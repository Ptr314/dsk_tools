#ifndef IMAGE_AGAT140_H
#define IMAGE_AGAT140_H

#include "dsk_tools/disk_image.h"

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
        uint8_t     track;          // 00
        uint8_t     sector;         // 01
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

#pragma pack(pop)

    class imageAgat140: public diskImage
    {
        protected:
            dsk_tools::Apple_DOS_VTOC * VTOC;
        public:
            imageAgat140(Loader * loader);
            virtual int check() override;
            virtual int open() override;
            virtual int translate_sector(int sector) override;
            virtual int dir(std::vector<dsk_tools::fileData> * files) override;
    };
}

#endif // IMAGE_AGAT140_H
