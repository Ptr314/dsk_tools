// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: FDD image conversion utility command-line tool

#include <iostream>
#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#endif

#include "cxxopts/cxxopts.hpp"
#include "bail.hpp"
#include  "fddconv.h"

#include "fs_host.h"
#include "host_helpers.h"
#include "dsk_tools/dsk_tools.h"

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

using namespace dsk_tools;

int main(int argc, char** argv)
{
    setupConsole();

    auto command {CLICommand::none};
    std::vector<std::string> add_values;
    bool output_expected = true;
    bool verbose = false;
    std::string input_file;
    std::string output_file;

    try {
        cxxopts::Options opts("fddconv", "FDD images conversion utility.");

        opts.add_options()
            ("v,verbose", "Print detailed information", cxxopts::value<bool>()->default_value("false"))
            ("input", "Input file", cxxopts::value<std::vector<std::string>>())
            ("o,output", "Output file", cxxopts::value<std::vector<std::string>>())
            ("l,ls", "List files", cxxopts::value<bool>()->default_value("false"))
            ("a,add", "File to add", cxxopts::value<std::vector<std::string>>())
            ("h,help", "Help");

        opts.parse_positional({"input"});

        const auto res = opts.parse(argc, argv);

        if (res["help"].as<bool>()) {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        if (res["verbose"].as<bool>()) {
            verbose = true;
        }

        if (res.count("input")) {
            if (verbose) {
                std::cout << "Input: ";
                for (const auto& input : res["input"].as<std::vector<std::string>>()) {
                    std::cout << "  " << input;
                }
                std::cout << std::endl;
            }
            input_file = res["input"].as<std::vector<std::string>>()[0];
        } else {
            return bail("No input file");
        }

        if (res["ls"].as<bool>()) {
            command = CLICommand::ls;
            output_expected = false;
        }

        if (output_expected) {
            if (res.count("output")) {
                if (verbose) {
                    std::cout << "Output:";
                    for (const auto& output : res["output"].as<std::vector<std::string>>()) {
                        std::cout << "  " << output;
                    }
                    std::cout << std::endl;
                }
            } else {
                bail("No output file");
            }
            output_file = res["output"].as<std::vector<std::string>>()[0];
        }

        if (res.count("add")) {
            add_values = res["add"].as<std::vector<std::string>>();
            if (!add_values.empty()) {
                command = CLICommand::add;
            }
        }
    }
    catch (const cxxopts::exceptions::exception& e) {
        return bail("Bad options: %s", e.what());
    }

    if (command == CLICommand::none) {
        return bail("No any command given");
    }

    std::string format_id;
    std::string type_id;
    std::string filesystem_id;
    const Result res = detect_fdd_type(input_file, format_id, type_id, filesystem_id, false);
    if (res) {
        if (verbose) {
            std::cout << "Format auto detection:" << std::endl;
            std::cout << "    Format = " << format_id << std::endl;
            std::cout << "    Type = " << type_id << std::endl;
            std::cout << "    Filesystem = " << filesystem_id << std::endl;
        }
    } else {
        return bail("Can't detect file type");
    }
    auto image = prepare_image(input_file, format_id, type_id);
    if (!image) return bail("Can't open image");

    auto load_res = image->load();
    if (!load_res) return bail("Can't load image");

    auto filesystem = prepare_filesystem(image.get(), filesystem_id);
    if (!filesystem) return bail("Can't prepare filesystem");

    auto open_res = filesystem->open();
    if (!open_res) return bail("Can't open filesystem");

    if (command == CLICommand::ls) {
        if (verbose) {
            std::cout << "Command: ls" << std::endl;
            std::cout << ">>>>>>>>--------------------------" << std::endl;
        }
        Files files;
        auto dir_res = filesystem->dir(files, true);
        if (!dir_res) {
            return bail("Can't list directory: %s", dir_res.message.c_str());
        }
        for (const auto& f : files) {
            std::cout << f.type_label << "\t" << f.size << "\t" << f.name << std::endl;
        }
        if (verbose) {
            std::cout << "<<<<<<<<--------------------------" << std::endl;
        }

    }

    if (command == CLICommand::add) {
        if (verbose) {
            std::cout << "Command: add" << std::endl;
            std::cout << "add_values: ";
        }
        auto host_fs = make_unique<fsHost>(nullptr);
        for (const auto& file_to_add : add_values) {
            if (verbose) {
                std::cout << "  " << file_to_add;
            }
            UniversalFile f;
            f.fs = FS::Host;
            f.name = file_to_add;
            f.metadata = strToBytes(file_to_add);
            BYTES data;
            auto get_res = host_fs->get_file(f, "", data);
            if (!get_res) bail("Can't read file %s", input_file.c_str());
            auto put_res = filesystem->put_file(f, "", data, true);
            if (!put_res) bail("Can't add file %s", input_file.c_str());
        }
        if (verbose) {
            std::cout  << std::endl;
        }
        std::cout << "Writing to output: " << output_file << std::endl;
        auto writer = make_unique<WriterRAW>(format_id, image.get());
        BYTES buffer;
        Result write_res = writer->write(buffer);
        if (!write_res) return bail("Can't get data: %s", write_res.message.c_str());
        UTF8_ofstream file(output_file, std::ios::binary);
        if (file.good()) {
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        }
    }

    return EXIT_SUCCESS;
}
