#ifndef WRITER_H
#define WRITER_H

#include <string>

#include "disk_image.h"

namespace dsk_tools {

    class Writer
    {
    protected:
        std::string     format_id;
        diskImage     * image;

    public:
        Writer(const std::string & format_id, diskImage *image_to_save);
        virtual ~Writer();

        virtual int write(const std::string & file_name);
        virtual int write(BYTES & buffer) = 0;
        virtual std::string get_default_ext() = 0;
        virtual int substitute_tracks(BYTES & buffer, BYTES & tmplt, const int numtracks) = 0;
        virtual int sector_raw2logic(int sector);
    };

}

#endif // WRITER_H
