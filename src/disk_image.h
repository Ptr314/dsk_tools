#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include "definitions.h"
#include "loader.h"

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
            BYTES buffer;
            Loader * loader = nullptr;
            int format_heads;
            int format_tracks;
            int format_sectors;
            int format_sector_size;
            int expected_size;
            int format_bitrate;
            int format_rpm;
            int format_track_encoding;
            int format_floppyinterfacemode;
            bool is_loaded;

        public:
            diskImage(Loader * loader);
            virtual ~diskImage();
            std::string file_name();
            virtual int check() = 0;                                            // Check physical image parameters
            virtual int load();
            // virtual int translate_sector_logic2raw(int sector);
            // virtual int translate_sector_raw2logic(int sector);
            virtual uint8_t *get_sector_data(int head, int track, int sector);      // Uses sector starnslation
            // virtual uint8_t *get_raw_sector_data(int head, int track, int sector);  // Do not uses translation
            bool get_loaded();
            int get_heads();
            int get_tracks();
            int get_sectors();
            int get_bitrate();
            int get_rpm();
            int get_track_encoding();
            int get_floppyinterfacemode();
            std::string get_type_id();
            BYTES * get_buffer();
    };
}

#endif // DISK_IMAGE_H
