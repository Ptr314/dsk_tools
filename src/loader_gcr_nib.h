#ifndef LOADER_GCR_NIB_H
#define LOADER_GCR_NIB_H

#include "loader_gcr.h"

namespace dsk_tools {

    class LoaderGCR_NIB:public LoaderGCR
    {
        public:
            LoaderGCR_NIB(std::string file_name, std::string format_id, std::string type_id);
            virtual ~LoaderGCR_NIB() = default;
        protected:
            virtual int get_track_len(int track) override;
    };

}


#endif // LOADER_GCR_NIB_H
