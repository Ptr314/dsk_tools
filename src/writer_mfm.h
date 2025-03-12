#ifndef WRITER_MFM_H
#define WRITER_MFM_H

#include "writer.h"

namespace dsk_tools {

    #define AGAT_140_GAP0    48
    #define AGAT_140_GAP1    6
    #define AGAT_140_GAP2    27
    #define AGAT_140_GAP3    (track_length - (AGAT_140_GAP0 + 16 * (3 + 8 + 3 + AGAT_140_GAP1 + 3 + 343 + 3 + AGAT_140_GAP2)))

    class WriterMFM:public Writer
    {
    protected:
        uint8_t m_volume_id;
        std::string m_interleaving_id;

        std::vector<int> sector_translation;

        void write_gcr62_track(BYTES &out, uint8_t track, int track_length);
        void write_gcr62_nic_track(BYTES &out, uint8_t track);
        void write_agat840_track(BYTES &out, uint8_t head, uint8_t track);
        void write_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t &last_byte);
        uint8_t write_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t &last_byte);
    public:
        virtual int sector_raw2logic(int sector) override;
        WriterMFM(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id, const std::string & interleaving_id);

    };

}

#endif // WRITER_MFM_H
