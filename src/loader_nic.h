#ifndef LOADER_NIC_H
#define LOADER_NIC_H

#include "loader_mfm.h"

namespace dsk_tools {

    class LoaderNIC:public LoaderMFM
    {
    public:
        LoaderNIC(const std::string & file_name, const std::string & format_id, const std::string & type_id);
    };

}

#endif // LOADER_NIC_H
