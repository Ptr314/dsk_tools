// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Converting an enum class to a bitwise flags container

#pragma once

//#include <type_traits>

// How to use:
/*
    enum class FileAccess : unsigned int {
        None    = 0,
        Read    = 1 << 0,
        Write   = 1 << 1,
        Execute = 1 << 2,
        All     = Read | Write | Execute
    };

    ENABLE_ENUM_FLAG_OPERATORS(FileAccess)

    int main() {
        FileAccess access = FileAccess::Read | FileAccess::Write;

        if (hasFlag(access, FileAccess::Read))
            std::cout << "Read is set\n";

        if (!hasFlag(access, FileAccess::Execute))
            std::cout << "Execute isn't set\n";

        access |= FileAccess::Execute;
        if (hasFlag(access, FileAccess::Execute))
            std::cout << "Execute is set now!\n";
    }
*/

namespace dsk_tools {

    #define ENABLE_ENUM_FLAG_OPERATORS(E)                                   \
        inline E operator~ (E a) noexcept {                                     \
            using T = typename std::underlying_type<E>::type;                   \
            return static_cast<E>(~static_cast<T>(a));                          \
    }                                                                       \
        inline E operator| (E a, E b) noexcept {                                \
            using T = typename std::underlying_type<E>::type;                   \
            return static_cast<E>(static_cast<T>(a) | static_cast<T>(b));       \
    }                                                                       \
        inline E operator& (E a, E b) noexcept {                                \
            using T = typename std::underlying_type<E>::type;                   \
            return static_cast<E>(static_cast<T>(a) & static_cast<T>(b));       \
    }                                                                       \
        inline E operator^ (E a, E b) noexcept {                                \
            using T = typename std::underlying_type<E>::type;                   \
            return static_cast<E>(static_cast<T>(a) ^ static_cast<T>(b));       \
    }                                                                       \
        inline E& operator|= (E& a, E b) noexcept { a = a | b; return a; }      \
        inline E& operator&= (E& a, E b) noexcept { a = a & b; return a; }      \
        inline E& operator^= (E& a, E b) noexcept { a = a ^ b; return a; }      \
        inline bool hasFlag(E value, E flag) noexcept {                         \
            using T = typename std::underlying_type<E>::type;                   \
            return (static_cast<T>(value) & static_cast<T>(flag)) != 0;         \
    }

}
