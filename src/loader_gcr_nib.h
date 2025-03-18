#ifndef LOADER_GCR_NIB_H
#define LOADER_GCR_NIB_H

#include "loader_gcr.h"

namespace dsk_tools {

    class LoaderGCR_NIB:public LoaderGCR
    {
        public:
            LoaderGCR_NIB(const std::string & file_name, const std::string & format_id, const std::string & type_id);
            virtual ~LoaderGCR_NIB() = default;
        protected:
            virtual int get_track_len(int track) override;
    };

}


#endif // LOADER_GCR_NIB_H
