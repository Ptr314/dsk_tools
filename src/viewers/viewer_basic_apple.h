#pragma once

#include "viewer_basic_agat.h"

namespace dsk_tools {

    class ViewerBASIC_Apple : public ViewerBASIC_Agat {
    public:
        static ViewerRegistrar<ViewerBASIC_Apple> registrar;

        std::string get_type() const override {return "BASIC";}
        std::string get_subtype() const override {return "APPLE";}
        int get_output_type() const override {return VIEWER_OUTPUT_TEXT;}

        std::string process_as_text(const BYTES & data, const std::string & cm_name) override;
    };

}
