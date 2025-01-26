#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include "dsk_tools/definitions.h"
#include "dsk_tools/loader.h"

namespace dsk_tools {

#define ISOIBM_MFM_ENCODING				0x00
#define AMIGA_MFM_ENCODING				0x01
#define ISOIBM_FM_ENCODING				0x02
#define EMU_FM_ENCODING					0x03
#define UNKNOWN_ENCODING				0xFF

#define IBMPC_DD_FLOPPYMODE				0x00
#define IBMPC_HD_FLOPPYMODE				0x01
#define ATARIST_DD_FLOPPYMODE			0x02
#define ATARIST_HD_FLOPPYMODE			0x03
#define AMIGA_DD_FLOPPYMODE				0x04
#define AMIGA_HD_FLOPPYMODE				0x05
#define CPC_DD_FLOPPYMODE				0x06
#define GENERIC_SHUGGART_DD_FLOPPYMODE	0x07
#define IBMPC_ED_FLOPPYMODE				0x08
#define MSX2_DD_FLOPPYMODE				0x09
#define C64_DD_FLOPPYMODE				0x0A
#define EMU_SHUGART_FLOPPYMODE			0x0B
#define S950_DD_FLOPPYMODE				0x0C
#define S950_HD_FLOPPYMODE				0x0D
#define DISABLE_FLOPPYMODE				0xFE


    class diskImage {
        protected:
            std::string type_id;
            std::vector<uint8_t> buffer;
            Loader * loader;
            int format_heads;
            int format_tracks;
            int format_sectors;
            int format_sector_size;
            int format_bitrate;
            int format_rpm;
            int format_track_encoding;
            int format_floppyinterfacemode;
            bool is_loaded;
            bool is_open;


        public:
            diskImage(Loader * loader);
            std::string file_name();
            virtual int check() = 0;                                            // Check physical image parameters
            virtual int open() = 0;                                             // Open and checks disk's filesystem
            virtual int get_capabilities() = 0;                                 // Get available functions
            virtual int dir(std::vector<dsk_tools::fileData> * files) = 0;      // List files
            virtual std::vector<uint8_t> get_file(const fileData & fd) = 0;
            virtual int load();
            virtual int translate_sector(int sector);
            virtual uint8_t *get_sector_data(int head, int track, int sector);
            int get_heads();
            int get_tracks();
            int get_sectors();
            int get_bitrate();
            int get_rpm();
            int get_track_encoding();
            int get_floppyinterfacemode();
            std::string get_type_id();
    };
}

#endif // DISK_IMAGE_H
