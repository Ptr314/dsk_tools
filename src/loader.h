#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <string>

#include "definitions.h"

namespace dsk_tools {

    class Loader
    {
        protected:
            std::string             file_name;
            std::string             format_id;
            std::string             type_id;
            uint32_t                image_size;
            bool                    loaded;

        public:
            Loader(std::string file_name, std::string format_id, std::string type_id);
            virtual ~Loader();

            std::string get_file_name();
            std::string get_type_id();
            virtual int load(BYTES & buffer) = 0;
            virtual std::string file_info() = 0;

    };


}

#endif // LOADER_H
