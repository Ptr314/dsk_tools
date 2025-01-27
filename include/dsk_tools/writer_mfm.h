#ifndef WRITER_MFM_H
#define WRITER_MFM_H

#include "dsk_tools/writer.h"

#define AGAT_140_GAP0    48
#define AGAT_140_GAP1    6
#define AGAT_140_GAP2    27
#define AGAT_140_GAP3    (6400 - (AGAT_140_GAP0 + 16 * (3 + 8 + 3 + AGAT_140_GAP1 + 3 + 343 + 3 + AGAT_140_GAP2)))


namespace dsk_tools {

    class WriterMFM:public Writer
    {
    protected:
        void write_gcr62_track(std::vector<uint8_t> &out, uint8_t track);
    public:
        WriterMFM(const std::string & format_id, diskImage *image_to_save);

    };

}

#endif // WRITER_MFM_H
