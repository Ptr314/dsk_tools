#pragma once

#include "viewer_text.h"

namespace dsk_tools {

    class ViewerBinary : public ViewerText {
    public:
        static ViewerRegistrar<ViewerBinary> registrar;

        std::string get_type() const override {return "BINARY";}
        std::string get_subtype() const override {return "";}

        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
