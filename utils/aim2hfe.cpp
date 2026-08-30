// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: AIM to HFE direct conversion command-line tool

#include <iostream>
#include <string>

#define BAIL_TOOL_NAME "aim2hfe"

#include "cxxopts/cxxopts.hpp"
#include "bail.hpp"
#include "cli_helpers.h"

#include "converters/aim2hfe.h"

using namespace dsk_tools;

namespace {

    // Replaces the input file extension (if any) with .hfe
    std::string default_output_name(const std::string & input_file)
    {
        const size_t slash = input_file.find_last_of("/\\");
        const size_t dot = input_file.find_last_of('.');
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash + 1))
            return input_file.substr(0, dot) + ".hfe";
        return input_file + ".hfe";
    }

}

int main(int argc, char** argv)
{
    setupConsole();

    bool verbose = false;
    std::string input_file;
    std::string output_file;

    // Parsing parameters ----------------------------------------------------

    try {
        cxxopts::Options opts("aim2hfe", "AIM to HFE direct conversion utility v1.0. https://github.com/Ptr314/dsk_tools");

        opts.add_options()
            ("v,verbose", "Print detailed information", cxxopts::value<bool>()->default_value("false"))
            ("input", "Input .aim file", cxxopts::value<std::string>())
            ("o,output", "Output .hfe file (default: input file name with .hfe extension)", cxxopts::value<std::string>())
            ("h,help", "Help");

        opts.parse_positional({"input"});
        opts.positional_help("<input.aim>");

        if (argc < 2) {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        const auto res = opts.parse(argc, argv);

        if (res["help"].as<bool>()) {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        verbose = res["verbose"].as<bool>();

        if (res.count("input"))
            input_file = res["input"].as<std::string>();
        else
            return bail("No input file");

        if (res.count("output"))
            output_file = res["output"].as<std::string>();
        else
            output_file = default_output_name(input_file);

        if (verbose) {
            std::cout << "Input: " << input_file << std::endl;
            std::cout << "Output: " << output_file << std::endl;
        }
    }
    catch (const cxxopts::exceptions::exception& e) {
        return bail("Bad options: %s", e.what());
    }

    if (input_file == output_file)
        return bail("Input and output files are the same");

    // Converting ------------------------------------------------------------

    AIM2HFEConverter aim2hfe;
    // The file-name overload lives in the base class and is hidden by the derived one
    Converter & converter = aim2hfe;

    std::string log;
    const Result res = converter.convert(input_file, output_file, log, verbose);

    if (!log.empty()) std::cout << log << std::flush;

    if (!res)
        return bail("Conversion failed : %s : %s", decode_error(res).c_str(), res.message.c_str());

    if (verbose) std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}