#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <string>
#include <vector>

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
            ~Loader();

            std::string get_file_name();
            std::string get_type_id();
            virtual int load(std::vector<uint8_t> * buffer) = 0;

    };


}

#endif // LOADER_H
