#ifndef WRITER_RAW_H
#define WRITER_RAW_H

#include "dsk_tools/writer.h"

namespace dsk_tools {

    class WriterRAW:public Writer
    {
    public:
        WriterRAW(const std::string & format_id, diskImage *image_to_save);
        virtual std::string get_default_ext() override;
        virtual int write(const std::string & file_name) override;

    };

}
#endif // WRITER_RAW_H
