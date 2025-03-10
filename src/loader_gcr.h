#ifndef LOADER_GCR_H
#define LOADER_GCR_H

#include "loader.h"

namespace dsk_tools {

    class LoaderGCR:public Loader
    {
    private:
        bool iterate_until(const BYTES & in, int & p, const uint8_t v);
    public:
        LoaderGCR(std::string file_name, std::string format_id, std::string type_id);
        ~LoaderGCR();
        virtual int load(std::vector<uint8_t> &buffer) override;
        virtual std::string file_info() override;
    };

}

#endif // LOADER_GCR_H
