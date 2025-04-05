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

    struct Agat_VTOC_Ex
    {
        uint32_t    free_sectors[64];
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

    struct FIL_header
    {
        uint8_t     name[30];
        uint8_t     tsl[9];
        uint8_t     type;
    };

    struct TS_PAIR
    {
        uint8_t     track;
        uint8_t     sector;
    };

    constexpr uint32_t VTOCMask140[32] = {
        0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, // 0..7
        0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, // 8..F
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    };

    constexpr uint32_t VTOCMask840[32] = {
         0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000, 0x00000100, 0x00000200, 0x00000400, // 00..07
         0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00000001, 0x00000002, 0x00000004, // 08..0F
         0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000000, 0x00000000, 0x00000000, // 10..14
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    };

    #pragma pack(pop)


    class fsDOS33: public fileSystem
    {
    protected:
        dsk_tools::Agat_VTOC * VTOC;
        TS_PAIR current_dir;
        std::vector<TS_PAIR> current_path;
        int attr_to_type(uint8_t a);
        virtual bool sector_is_free(int head, int track, int sector) override;
        virtual void sector_free(int head, int track, int sector) override;
        virtual bool sector_occupy(int head, int track, int sector) override;
        virtual int free_sectors() override;
        virtual uint32_t * track_map(int track);

    public:
        fsDOS33(diskImage * image);
        virtual int open() override;
        virtual int get_capabilities() override;
        virtual void cd(const dsk_tools::fileData & dir) override;
        virtual void cd_up() override;
        virtual int dir(std::vector<dsk_tools::fileData> * files, bool show_deleted = true) override;
        virtual BYTES get_file(const fileData & fd) override;
        virtual std::string file_info(const fileData & fd) override;
        virtual bool file_delete(const fileData & fd) override;
        virtual int file_add(const std::string & file_name, const std::string & format_id) override;
        virtual std::vector<std::string> get_save_file_formats() override;
        virtual std::vector<std::string> get_add_file_formats() override;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
        virtual std::string information() override;
    };
}

#endif // FS_DOS33_H
