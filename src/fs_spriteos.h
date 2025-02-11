#ifndef FS_SPRITEOS_H
#define FS_SPRITEOS_H

#include "filesystem.h"

    namespace dsk_tools {

    #pragma pack(push, 1)

    struct SPRITE_OS_DPB_DISK {
        uint8_t HEADER[4];
        uint8_t VOLUME;
        uint8_t TYPE;
        uint8_t DSIDE;
        uint8_t TSIZE;
        uint16_t DSIZE;
        uint16_t MAXBLOK;
        uint8_t VTOCADR;
    };
    struct SPRITE_OS_DIR_ENTRY {
        uint8_t NAME[15];            // 00-0E: имя файла, дополненное пробелами в конце
        uint8_t STATUS;              // 0F: шкала атрибутов файла
        uint8_t LEVEL;               // 10: число уровней БC в файле (0-3)
        uint16_t INFADR;             // 11-12: ссылка (номер блока) на БC высшего уровня
        uint16_t BLOCKS;             // 13-14: число фактически занятых блоков, считая БC
        uint16_t RECLEN;             // 15-16: длина записи файла в байтах
        uint16_t DATE;               // 17-18: дата создания или послед. изменения файла
        uint8_t FILELEN[3];          // 19-1B: длина файла (позиция конца послед. записи)
        uint8_t USRINF[4];             // 1C-1F: пользовательская информация о файле
    };

    #define SPRITE_OS_DIR_LENGTH (256/sizeof(SPRITE_OS_DIR_ENTRY))

    typedef SPRITE_OS_DIR_ENTRY DIR_BLOCK[SPRITE_OS_DIR_LENGTH];

    #pragma pack(pop)


    class fsSpriteOS: public fileSystem
    {
    protected:
        SPRITE_OS_DPB_DISK DPB;
        SPRITE_OS_DIR_ENTRY CURRENT_DIR;
        std::vector<SPRITE_OS_DIR_ENTRY> current_path;
        void load_file(const SPRITE_OS_DIR_ENTRY & dir_entry, BYTES & out);
    public:
        fsSpriteOS(diskImage * image);
        virtual int open() override;
        virtual int get_capabilities() override;
        virtual void cd(const dsk_tools::fileData & dir) override;
        virtual int dir(std::vector<dsk_tools::fileData> * files) override;
        virtual BYTES get_file(const fileData & fd) override;
        virtual std::string file_info(const fileData & fd) override;
        virtual std::vector<std::string> get_save_file_formats() override;
        virtual int save_file(const std::string & format_id, const std::string & file_name, const fileData & fd) override;
    };
}

#endif // FS_SPRITEOS_H
