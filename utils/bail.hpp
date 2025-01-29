#ifndef BAIL_HPP
#define BAIL_HPP

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

static void vprinterr(const char* format, std::va_list vlist) noexcept
{
    std::fputs("fddconv: \033[1;31merror:\033[0m ", stderr);
    std::vfprintf(stderr, format, vlist);
    std::fputs("\n", stderr);
}


static int bail(const char* format, ...) noexcept
{
    std::va_list args;
    va_start(args, format);
    vprinterr(format, args);
    va_end(args);
    return EXIT_FAILURE;
}

#endif // BAIL_HPP
