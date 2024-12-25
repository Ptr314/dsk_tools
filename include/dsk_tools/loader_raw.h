#ifndef LOADER_RAW_H
#define LOADER_RAW_H

#include "dsk_tools/loader.h"

namespace dsk_tools {

    class LoaderRAW:public Loader
    {
        protected:
            bool msb_first;
        public:
            LoaderRAW(std::string file_name, std::string format_id, std::string type_id, bool msb_first);
            virtual int load(std::vector<uint8_t> * buffer) override;
    };

}

#endif // LOADER_RAW_H
