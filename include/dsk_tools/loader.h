#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <string>
#include <vector>

namespace dsk_tools {

    #define FDD_LOAD_OK                 0
    #define FDD_LOAD_ERROR              1
    #define FDD_LOAD_SIZE_SMALLER       2
    #define FDD_LOAD_SIZE_LARGER        3
    #define FDD_LOAD_PARAMS_MISMATCH    4
    #define FDD_LOAD_INCORRECT_FILE     5
    #define FDD_LOAD_FILE_CORRUPT       6

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

            virtual int load(std::vector<uint8_t> * buffer) = 0;
            //uint8_t     *get_sector_data(uint8_t head, uint8_t track, uint8_t sector);

    };


}

#endif // LOADER_H
