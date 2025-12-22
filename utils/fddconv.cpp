// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: FDD image conversion utility command-line tool

#include <iostream>
#include <algorithm>

#include "cxxopts/cxxopts.hpp"
#include "bail.hpp"
#include "cli_helpers.h"

#include "fs_host.h"
#include "host_helpers.h"
#include "dsk_tools/dsk_tools.h"

using namespace dsk_tools;




int main(int argc, char** argv)
{
    setupConsole();

    CLICommand command = CLICommand::none;
    std::vector<std::string> add_values;
    std::vector<std::string> delete_values;
    std::vector<std::string> extract_values;
    bool output_expected = true;
    bool verbose = false;
    bool is_bin = false;
    std::string input_file;
    std::string output_file;
    uint8_t volume_id = 254;
    std::string format_id;
    std::string type_id;
    std::string filesystem_id;
    std::string type_str;
    std::string fs_str;


    // Parsing parameters ----------------------------------------------------

    try {
        cxxopts::Options opts("fddconv", "FDD images conversion utility.");

        opts.add_options()
            ("v,verbose", "Print detailed information", cxxopts::value<bool>()->default_value("false"))
            ("input", "Input file", cxxopts::value<std::string>())
            ("o,output", "Output file", cxxopts::value<std::string>())
            ("l,ls", "List files", cxxopts::value<bool>()->default_value("false"))
            ("a,add", "File to add", cxxopts::value<std::vector<std::string>>())
            ("e,extract", "File to extract from the disk", cxxopts::value<std::vector<std::string>>())
            ("b,binary", "Save extracted files as binary, not FIL (For Apple DOS)", cxxopts::value<bool>()->default_value("false"))
            ("d,delete", "File to delete", cxxopts::value<std::vector<std::string>>())
            ("m,volume", "Volume id - decimal (i.e. 254) or hex (i.e. $FE)", cxxopts::value<std::string>())
            ("f,input_format", "Input file format DDD:FFF \n"
                                        "DDD: a140 (Apple/Agat 140k), \n"
                                        "     a840 (Agat 840k). \n"
                                        "FFF: dos33 (Apple DOS), \n"
                                        "     sos (Sprite OS), \n"
                                        "     cpm, cpm-do, cpm-po (CP/M raw, DOS sectors, ProDOS sectors)",
                                        cxxopts::value<std::string>())
            ("h,help", "Help");

        opts.parse_positional({"input"});

        const auto res = opts.parse(argc, argv);

        if (res["help"].as<bool>()) {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        verbose = res["verbose"].as<bool>();
        is_bin = res["binary"].as<bool>();

        if (res.count("input")) {
            input_file = res["input"].as<std::string>();
            if (verbose) std::cout << "Input: " << input_file << std::endl;
        } else
            return bail("No input file");

        if (res["ls"].as<bool>()) {
            command = CLICommand::ls;
            output_expected = false;
        }

        if (res.count("add")) {
            add_values = res["add"].as<std::vector<std::string>>();
            if (!add_values.empty()) {
                command = CLICommand::add;
            }
        }

        if (res.count("delete")) {
            delete_values = res["delete"].as<std::vector<std::string>>();
            if (!delete_values.empty()) {
                command = CLICommand::del;
            }
        }

        if (res.count("extract")) {
            extract_values = res["extract"].as<std::vector<std::string>>();
            if (!extract_values.empty()) {
                command = CLICommand::extract;
                output_expected = false;
            }
        }

        if (res.count("volume")) {
            const std::string vid_str = res["volume"].as<std::string>();
            volume_id = parse_number(vid_str) & 0xFF;
            if (verbose) std::cout << "Volume id: " << std::to_string(volume_id) << " ($" << int_to_hex(volume_id) << ")" <<std::endl;
        }

        if (res.count("input_format")) {
            const std::string format_str = res["input_format"].as<std::string>();
            size_t pos = format_str.find(':');
            if (pos != std::string::npos) {
                type_str = format_str.substr(0, pos);
                fs_str = format_str.substr(pos + 1);
                std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::tolower);
                std::transform(fs_str.begin(), fs_str.end(), fs_str.begin(), ::tolower);
            } else return bail("Incorrect input format");
        }

        if (output_expected) {
            if (res.count("output")) {
                output_file = res["output"].as<std::string>();
                if (verbose) std::cout << "Output: " << output_file << std::endl;
            } else
                return bail("No output file");
        }


    }
    catch (const cxxopts::exceptions::exception& e) {
        return bail("Bad options: %s", e.what());
    }

    if (command == CLICommand::none) {
        if (verbose) std::cout << "No any command given, just converting" << std::endl;
        // return bail("No any command given");
    }

    // Loading input file ----------------------------------------------------

    if (type_str.empty() && fs_str.empty()) {
        const Result res = detect_fdd_type(input_file, format_id, type_id, filesystem_id, false);
        if (res) {
            if (verbose) {
                std::cout << "Format auto detection:" << std::endl;
                std::cout << "    Format = " << format_id << std::endl;
                std::cout << "    Type = " << type_id << std::endl;
                std::cout << "    Filesystem = " << filesystem_id << std::endl;
            }
        } else {
            return bail("Input file error : %s : %s", decode_error(res).c_str(), res.message.c_str());
        }
    } else {
        const Result res = detect_fdd_type(input_file, format_id, type_id, filesystem_id, true);
        if (res) {
            if (type_str == "a140") type_id = "TYPE_AGAT_140";
            if (type_str == "a840") type_id = "TYPE_AGAT_840";
            if (type_id.empty()) return bail("Incorrect disk format");
            if (fs_str == "dos33") filesystem_id = "FILESYSTEM_DOS33";
            if (fs_str == "sos") filesystem_id = "FILESYSTEM_SPRITE_OS";
            if (fs_str == "cpm") filesystem_id = "FILESYSTEM_CPM_RAW";
            if (fs_str == "cpm-do") filesystem_id = "FILESYSTEM_CPM_DOS";
            if (fs_str == "cpm-po") filesystem_id = "FILESYSTEM_CPM_PRODOS";
            if (filesystem_id.empty()) return bail("Incorrect type of filesystem");
            if (verbose) {
                std::cout << "Input file parameters:" << std::endl;
                std::cout << "    Format = " << format_id << std::endl;
                std::cout << "    Type = " << type_id << std::endl;
                std::cout << "    Filesystem = " << filesystem_id << std::endl;
            }
        } else {
            return bail("Input file error : %s : %s", decode_error(res).c_str(), res.message.c_str());
        }
    }
    auto image = prepare_image(input_file, format_id, type_id);
    if (!image) return bail("Can't open image");

    auto load_res = image->load();
    if (!load_res) return bail("Can't load image : %s : %s", decode_error(load_res).c_str(), load_res.message.c_str());

    auto filesystem = prepare_filesystem(image.get(), filesystem_id);
    if (!filesystem) return bail("Can't prepare filesystem");

    auto open_res = filesystem->open();
    if (!open_res) return bail("Can't open filesystem : %s : %s", decode_error(open_res).c_str(), open_res.message.c_str());

    // Performing commands ----------------------------------------------------

    // LS

    if (command == CLICommand::ls) {
        if (verbose) {
            std::cout << "Command: ls" << std::endl;
            std::cout << ">>>>>>>>--------------------------" << std::endl;
        }
        Files files;
        auto dir_res = filesystem->dir(files, true);
        if (!dir_res) {
            return bail("Can't list directory : %s : %s", decode_error(dir_res).c_str(), dir_res.message.c_str());
        }
        for (const auto& f : files) {
            std::cout << f.type_label << "\t" << f.size << "\t" << f.name << std::endl;
        }
        if (verbose) {
            std::cout << "<<<<<<<<--------------------------" << std::endl;
        }
    }

    // ADD

    if (command == CLICommand::add) {
        if (verbose) {
            std::cout << "Command: add" << std::endl;
            std::cout << "adding: ";
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
    }

    // DELETE

    if (command == CLICommand::del) {
        if (verbose) {
            std::cout << "Command: delete" << std::endl;
            std::cout << "deleting: ";
        }
        for (const auto& file_to_delete : delete_values) {
            if (verbose) {
                std::cout << "  " << file_to_delete;
            }
            UniversalFile f;
            auto find_res = filesystem->find_file(to_upper(file_to_delete), f);
            if (!find_res) {return bail("Can't find file: %s", file_to_delete.c_str());}
            auto del_res = filesystem->delete_file(f);
            if (!del_res) {return bail("Can't delete file: %s", file_to_delete.c_str());}
        }
        if (verbose) {
            std::cout  << std::endl;
        }
    }

    if (command == CLICommand::extract) {
        if (verbose) {
            std::cout << "Command: extract" << std::endl;
            std::cout << "extracting: ";
        }
        auto host_fs = make_unique<fsHost>(nullptr);
        for (const auto& file_to_extract : extract_values) {
            if (verbose) std::cout << "  " << file_to_extract;
            UniversalFile f;
            const auto find_res = filesystem->find_file(to_upper(file_to_extract), f);
            if (!find_res) {return bail("Can't find file '%s': %s : %s", file_to_extract.c_str(), decode_error(find_res).c_str(), find_res.message.c_str());}
            BYTES buffer;
            std::string out_format;
            if (filesystem_id == "FILESYSTEM_DOS33") out_format = is_bin ? "FILE_BINARY" : "FILE_FIL";
            const auto get_res = filesystem->get_file(f, out_format, buffer);
            if (!get_res) {return bail("Can't read file '%s': %s : %s", file_to_extract.c_str(), decode_error(get_res).c_str(), get_res.message.c_str());}
            if (out_format == "FILE_FIL") f.name += ".fil";
            if (verbose) std::cout << " -> " << f.name;
            const auto put_res = host_fs->put_file(f, "", buffer, true);
            if (!put_res) {return bail("Can't write file '%s': %s : %s", file_to_extract.c_str(), decode_error(put_res).c_str(), put_res.message.c_str());}
        }
        if (verbose) {
            std::cout  << std::endl;
        }
    }

    // Writing results ----------------------------------------------------

    if (output_expected) {
        Result write_res = write_output_file(output_file, format_id, volume_id, image.get(), verbose);
        if (!write_res) return bail("Can't write output file : %s : %s", decode_error(write_res).c_str(), write_res.message.c_str());
    }

    return EXIT_SUCCESS;
}
