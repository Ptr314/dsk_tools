#ifndef LOADER_HXC_HFE_H
#define LOADER_HXC_HFE_H

#include "loader.h"

namespace dsk_tools {

    class LoaderHXC_HFE:public Loader
    {
    public:
        LoaderHXC_HFE(const std::string & file_name, const std::string & format_id, const std::string & type_id);
        virtual int load(BYTES & buffer) override;
        virtual std::string file_info() override;
    };

}


#endif // LOADER_HXC_HFE_H
