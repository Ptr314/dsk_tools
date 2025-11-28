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

int main(int argc, char** argv)
{
    setupConsole();
    try {
        cxxopts::Options opts("fddconv", "FDD images conversion utility.");

        opts.add_options()
            //("v,verbose", "Вывод отладочной информации", cxxopts::value<bool>()->default_value("false"))
            ("h,help", "Вывести справку")
            ("input", "Входные файлы", cxxopts::value<std::vector<std::string>>());

        opts.parse_positional({"input"});

        auto res = opts.parse(argc, argv);
        if (res["help"].as<bool>())
        {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        if (res.count("input")) {
            std::cout << "Позиционные аргументы:" << std::endl;
            for (const auto& input : res["input"].as<std::vector<std::string>>()) {
                std::cout << "  " << input << std::endl;
            }
        } else {
            bail("No input file");
        }

    }
    catch (const cxxopts::exceptions::exception& e)
    {
        return bail("Bad options: %s", e.what());
    }
}
