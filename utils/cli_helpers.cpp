// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: FDD image conversion utility command-line tool

#ifdef _WIN32
#include <windows.h>
#endif

#include "host_helpers.h"
#include "cli_helpers.h"

namespace dsk_tools {

    void setupConsole() {
        #ifdef _WIN32
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode;
            if (GetConsoleMode(hOut, &mode)) {
                // ENABLE_VIRTUAL_TERMINAL_PROCESSING not available in older Windows SDK versions
        #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
        #endif
                SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        #endif
    }

    std::string decode_error(const Result& result) {
        std::string error;
        switch (result.code) {
            case ErrorCode::Ok:
                error = "No error";
                break;
            case ErrorCode::NotImplementedYet:
                error = "Not implemented yet";
                break;
            case ErrorCode::NotFound:
                error = "Item not found";
                break;
            case ErrorCode::LoadError:
                error = "Error loading disk image file";
                break;
            case ErrorCode::LoadSizeMismatch:
                error = "File size does not match expected disk image size";
                break;
            case ErrorCode::LoadParamsMismatch:
                error = "File parameters do not match disk image parameters";
                break;
            case ErrorCode::LoadIncorrectFile:
                error = "File format is not recognized";
                break;
            case ErrorCode::LoadDataCorrupt:
                error = "Disk image data is corrupted";
                break;
            case ErrorCode::OpenNotLoaded:
                error = "Image file is not loaded";
                break;
            case ErrorCode::OpenBadFormat:
                error = "Unrecognized disk format or disk is damaged";
                break;
            case ErrorCode::CreateError:
                error = "Error creating file";
                break;
            case ErrorCode::WriteError:
                error = "Error writing file";
                break;
            case ErrorCode::WriteUnsupported:
                error = "Writing to this format is not supported";
                break;
            case ErrorCode::WriteIncorrectTemplate:
                error = "The selected template cannot be used - it must be the same type and size as the target";
                break;
            case ErrorCode::WriteIncorrectSource:
                error = "Incorrect source data for tracks replacement";
                break;
            case ErrorCode::DirError:
                error = "Error creating a directory";
                break;
            case ErrorCode::DirErrorSpace:
                error = "No enough free space";
                break;
            case ErrorCode::DirErrorAllocateDirEntry:
                error = "Can't allocate a directory entry";
                break;
            case ErrorCode::DirErrorAllocateSector:
                error = "Can't allocate a sector";
                break;
            case ErrorCode::DirNotEmpty:
                error = "Directory is not empty";
                break;
            case ErrorCode::FileDeleteError:
                error = "Error deleting file";
                break;
            case ErrorCode::FileAddError:
                error = "Error adding file";
                break;
            case ErrorCode::FileAddErrorAllocateDirEntry:
                error = "Can't allocate a directory entry";
                break;
            case ErrorCode::FileAddErrorAllocateSector:
                error = "Can't allocate a sector";
                break;
            case ErrorCode::FileAddErrorSpace:
                error = "No enough free space";
                break;
            case ErrorCode::FileRenameError:
                error = "Error renaming file";
                break;
            case ErrorCode::FileIncorrectFS:
                error = "File is not compatible with this filesystem";
                break;
            case ErrorCode::ReadError:
                error = "Error reading file";
                break;
            case ErrorCode::FileNotFound:
                error = "File not found";
                break;
            case ErrorCode::FileAlreadyExists:
                error = "File already exists";
                break;
            case ErrorCode::DirAlreadyExists:
                error = "Directory already exists";
                break;
            case ErrorCode::InvalidName:
                error = "Invalid name";
                break;
            case ErrorCode::DetectError:
                error = "Error detecting disk image format";
                break;
            case ErrorCode::FileMetadataError:
                error = "File metadata error";
                break;
            default:
                error = "Unknown error";
                break;
        }
        return error;
    }

    Result write_output_file(const std::string & output_file, const std::string & format_id, diskImage * image, const bool verbose)
    {
        if (verbose) std::cout << "Writing to output: " << output_file << std::endl;
        #ifdef _MSC_VER
            const auto writer = std::make_unique<WriterRAW>(format_id, image);
        #else
            const auto writer = make_unique<WriterRAW>(format_id, image);
        #endif
        BYTES buffer;
        const Result write_res = writer->write(buffer);
        if (!write_res) return Result::error(ErrorCode::WriteError);
        UTF8_ofstream file(output_file, std::ios::binary);
        if (file.good()) {
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (file.good()) return Result::ok();
        }
        return Result::error(ErrorCode::WriteError);
    }
}
