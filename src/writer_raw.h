#ifndef WRITER_RAW_H
#define WRITER_RAW_H

#include "writer.h"

namespace dsk_tools {

    class WriterRAW:public Writer
    {
    public:
        WriterRAW(const std::string & format_id, diskImage *image_to_save);
        virtual std::string get_default_ext() override;
        virtual int write(BYTES & buffer) override;
        virtual int substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) override;
    };

}
#endif // WRITER_RAW_H
