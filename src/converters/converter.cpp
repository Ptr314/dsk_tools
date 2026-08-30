// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Direct converters base class

#include "converter.h"

#include "host_helpers.h"

namespace dsk_tools
{
    Result Converter::convert(const std::string & file_in, const std::string & file_out, std::string & log, bool verbose)
    {
        BYTES in, out;

        if (verbose) log += "Reading: " + file_in + "\n";

        UTF8_ifstream fi(file_in, std::ios::binary);
        if (!fi.good())
            return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Cannot open file"));

        fi.seekg(0, std::ios::end);
        auto fsize = fi.tellg();
        fi.seekg(0, std::ios::beg);

        if (fsize <= 0)
            return Result::error(ErrorCode::LoadSizeMismatch, QT_TRANSLATE_NOOP("errors", "The file is empty"));

        in.resize(static_cast<size_t>(fsize));
        fi.read(reinterpret_cast<char*>(in.data()), static_cast<std::streamsize>(fsize));
        if (!fi.good())
            return Result::error(ErrorCode::ReadError, QT_TRANSLATE_NOOP("errors", "Cannot read file"));

        fi.close();

        if (verbose) log += "Read: " + std::to_string(in.size()) + " byte(s)\n";

        const Result res = convert(in, out, log, verbose);
        if (!res) return res;

        if (verbose) log += "Writing: " + file_out + " (" + std::to_string(out.size()) + " byte(s))\n";

        UTF8_ofstream fo(file_out, std::ios::binary);
        if (!fo.good())
            return Result::error(ErrorCode::CreateError, QT_TRANSLATE_NOOP("errors", "Cannot create file"));

        fo.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
        if (!fo.good())
            return Result::error(ErrorCode::WriteError, QT_TRANSLATE_NOOP("errors", "Cannot write file"));

        fo.close();

        return Result::ok();
    }
}