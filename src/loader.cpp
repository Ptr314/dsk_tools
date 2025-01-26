#include "dsk_tools/loader.h"

namespace dsk_tools {
    Loader::Loader(std::string file_name, std::string format_id, std::string type_id):
          file_name(file_name)
        , format_id(format_id)
        , type_id(type_id)
        , loaded(false)

    {}

    std::string Loader::get_file_name()
    {
        return file_name;
    }

    std::string Loader::get_type_id()
    {
        return type_id;
    }

}
