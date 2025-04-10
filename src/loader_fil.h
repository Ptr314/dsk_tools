#ifndef LOADER_FIL_H
#define LOADER_FIL_H

#include "loader.h"

namespace dsk_tools {

    class LoaderFIL:public Loader
    {
    public:
        LoaderFIL(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual int load(BYTES & buffer) override;
        virtual std::string file_info() override;
    };

}

#endif // LOADER_FIL_H
