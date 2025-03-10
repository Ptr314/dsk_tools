#ifndef LOADER_AIM_H
#define LOADER_AIM_H

#include "loader.h"

namespace dsk_tools {

    class LoaderAIM:public Loader
    {
    private:
        bool iterate_until(const std::vector<uint16_t> &in, int &p, const uint8_t v);
    protected:
        bool msb_first;
    public:
        LoaderAIM(std::string file_name, std::string format_id, std::string type_id);
        ~LoaderAIM();
        virtual int load(std::vector<uint8_t> &buffer) override;
        virtual std::string file_info() override;
    };

}
#endif // LOADER_AIM_H
