#pragma once

#include "viewer.h"

namespace dsk_tools {

    class ViewerText : public Viewer {
    public:
        static ViewerRegistrar<ViewerText> registrar;

        std::string get_type() const override {return "TEXT";}
        std::string get_subtype() const override {return "";}
        int get_output_type() const override {return VIEWER_OUTPUT_TEXT;}

        virtual std::string process_as_text(const BYTES & data, const std::string & cm_name) override;

    protected:
        const std::string (*charmap)[256];
        std::set<uint8_t> crlf;
        std::set<uint8_t> ignore = {};
        std::set<uint8_t> txt_end = {};
        int tab = 0;
        virtual void init_charmap(std::string cm_name);

    };

}
