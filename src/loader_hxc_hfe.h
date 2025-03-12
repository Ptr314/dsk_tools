#ifndef LOADER_HXC_HFE_H
#define LOADER_HXC_HFE_H

#include "loader.h"

namespace dsk_tools {

    class LoaderHXC_HFE:public Loader
    {
    public:
        LoaderHXC_HFE(std::string file_name, std::string format_id, std::string type_id);
        virtual int load(BYTES & buffer) override;
        virtual std::string file_info() override;
    };

}


#endif // LOADER_HXC_HFE_H
