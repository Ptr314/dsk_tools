#ifndef LOADER_RAW_H
#define LOADER_RAW_H

#include "loader.h"

namespace dsk_tools {

    class LoaderRAW:public Loader
    {
        public:
            LoaderRAW(const std::string & file_name, const std::string & format_id, const std::string & type_id);
            virtual int load(BYTES & buffer) override;
            virtual std::string file_info() override;

    };

}

#endif // LOADER_RAW_H
