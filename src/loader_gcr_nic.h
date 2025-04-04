#ifndef LOADER_GCR_NIC_H
#define LOADER_GCR_NIC_H

#include "loader_gcr.h"

namespace dsk_tools {

    class LoaderGCR_NIC:public LoaderGCR
    {
        public:
        LoaderGCR_NIC(const std::string &file_name, const std::string &format_id, const std::string &type_id);
            virtual ~LoaderGCR_NIC() = default;
        protected:
            virtual int get_track_len(int track) override;
    };

}


#endif // LOADER_GCR_NIC_H
