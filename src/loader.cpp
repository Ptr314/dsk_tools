#include "loader.h"

namespace dsk_tools {
    Loader::Loader(const std::string & file_name, const std::string & format_id, const std::string & type_id):
          file_name(file_name)
        , format_id(format_id)
        , type_id(type_id)
        , loaded(false)
    {}

}
