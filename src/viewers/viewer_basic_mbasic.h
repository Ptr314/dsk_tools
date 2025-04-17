#pragma once

#include "viewer.h"
#include "viewers/viewer_text.h"

namespace dsk_tools {

    class ViewerBASIC_MBASIC : public ViewerText {
    public:
        static ViewerRegistrar<ViewerBASIC_MBASIC> registrar;

        std::string get_type() const override {return "BASIC";}
        std::string get_subtype() const override {return "MBASIC";}
        int get_output_type() const override {return VIEWER_OUTPUT_TEXT;}

        std::string process_as_text(const BYTES &data, const std::string &cm_name) override;

    protected:
        std::string convert_tokenized(const BYTES & data, const std::string & cm_name, const std::array<const char*, 128> & tokens);
    };

}
