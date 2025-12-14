// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Selectable options base class

#pragma once

#include <string>
#include <vector>
#include <map>

namespace dsk_tools {

    struct ViewerSelectorOption {
        std::string id;
        std::string title;
        std::string custom;
    };

    typedef std::vector<ViewerSelectorOption> ViewerSelectorOptions;

    typedef std::map<std::string, std::string> ViewerSelectorValues;

    class ViewerSelector {
    public:
        virtual ~ViewerSelector() = default;

        virtual std::string get_id() = 0;
        virtual std::string get_title() = 0;
        virtual std::string get_type() {return "dropdown";}
        virtual std::string get_icon() = 0;
        virtual bool has_customs() = 0;
        virtual ViewerSelectorOptions get_options() = 0;
    };

    typedef std::vector<std::unique_ptr<ViewerSelector>> ViewerSelectors;

}
