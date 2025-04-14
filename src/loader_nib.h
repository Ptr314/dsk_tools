#ifndef LOADER_NIB_H
#define LOADER_NIB_H

#include "loader_mfm.h"

namespace dsk_tools {

    class LoaderNIB:public LoaderMFM
    {
    public:
        LoaderNIB(const std::string & file_name, const std::string & format_id, const std::string & type_id);
    };

}

#endif // LOADER_NIB_H
