// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mike Shevchenko <mike.shevchenko@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Apple II and Agat 140 Kb disks in HFE through the public API
//
//   test_agat140 [scratch directory]
//
// A DOS 3.3 disk made up here is written to an HFE, which goes through
// detect_fdd_type(), prepare_image(), the filesystem and file_info() as the GUI
// uses them; then again with the transitions in the first cell of each pair and
// the encoding named, as the HxC software writes such files

#include <cstdio>
#include <fstream>
#include <string>

#include "dsk_tools/dsk_tools.h"

using namespace dsk_tools;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void write_all(const std::string & name, const BYTES & data)
{
    std::ofstream out(name, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
}

// A DOS 3.3 disk with one file, an Applesoft program HELLO: 10 PRINT "HELLO",
// 20 END. The VTOC and the catalog on track 17, the file on track 18, no DOS
// on the disk but the first byte 01 a boot sector starts with
static BYTES dos33_disk()
{
    BYTES disk(35 * 16 * 256, 0);
    auto sector = [&](int t, int s) { return disk.data() + (t * 16 + s) * 256; };
    disk[0] = 0x01;
    uint8_t * vtoc = sector(17, 0);
    vtoc[0x01] = 17;            // the catalog's first track and sector
    vtoc[0x02] = 15;
    vtoc[0x03] = 3;             // DOS release
    vtoc[0x06] = 0xFE;          // volume
    vtoc[0x27] = 122;           // track/sector pairs in a list
    vtoc[0x30] = 18;            // the track allocated last, and the way onward
    vtoc[0x31] = 1;
    vtoc[0x34] = 35;            // tracks, sectors a track, 256 bytes a sector
    vtoc[0x35] = 16;
    vtoc[0x37] = 1;
    for (int t = 0; t < 35; t++) {
        // A bit for each free sector, 15 to 0: the catalog takes track 17, the file two sectors
        const uint16_t free = (t == 17) ? 0x0000 : (t == 18) ? 0x3FFF : 0xFFFF;
        vtoc[0x38 + 4 * t] = (uint8_t)(free >> 8);
        vtoc[0x39 + 4 * t] = (uint8_t)free;
    }
    for (int s = 15; s > 1; s--) {
        sector(17, s)[1] = 17;
        sector(17, s)[2] = (uint8_t)(s - 1);
    }
    uint8_t * entry = sector(17, 15) + 0x0B;
    entry[0] = 18;              // the track/sector list
    entry[1] = 15;
    entry[2] = 0x02;            // Applesoft
    const char name[] = "HELLO";
    for (int i = 0; i < 30; i++) entry[3 + i] = (uint8_t)((i < 5 ? name[i] : ' ') | 0x80);
    entry[0x21] = 2;            // sectors
    sector(18, 15)[0x0C] = 18;
    sector(18, 15)[0x0D] = 14;
    // The program as Applesoft keeps it from $801 on, after its length: each line
    // the address of the next one, its number, its tokens (PRINT BA, END 80), 0
    const uint8_t program[] = {
        21, 0,
        0x0E, 0x08, 10, 0, 0xBA, '"', 'H', 'E', 'L', 'L', 'O', '"', 0,
        0x14, 0x08, 20, 0, 0x80, 0,
        0, 0
    };
    for (size_t i = 0; i < sizeof(program); i++) sector(18, 14)[i] = program[i];
    return disk;
}

// The HFE goes through what the GUI does with a file it opens
static void check_hfe(const std::string & name, const BYTES & disk, const char * what)
{
    const DiskDefs defs;
    std::string format_id, type_id, fs_id;
    CHECK(detect_fdd_type(name, format_id, type_id, fs_id), "%s: not detected", what);
    printf("  %s: detected %s, %s\n", what, type_id.c_str(), fs_id.c_str());
    CHECK(format_id == "FILE_HXC_HFE" && type_id == "TYPE_AGAT_140" && fs_id == "FILESYSTEM_DOS33",
          "%s: detected as %s, %s, %s", what, format_id.c_str(), type_id.c_str(), fs_id.c_str());
    auto image = prepare_image(name, format_id, type_id, defs);
    if (!image || !image->load()) {
        CHECK(false, "%s: does not load", what);
        return;
    }
    CHECK(*image->get_buffer() == disk, "%s: the disk differs", what);

    auto fs = prepare_filesystem(image.get(), fs_id, defs);
    std::vector<UniversalFile> files;
    CHECK(fs && fs->open() && fs->dir(files, false), "%s: the catalog does not open", what);
    CHECK(files.size() == 1 && files[0].name == "HELLO", "%s: %zu files", what, files.size());

    const std::string info = image->file_info();
    CHECK(info.find("{$INDEX_MARK} ($D5 $AA $96)") != std::string::npos
          && info.find("ERROR") == std::string::npos, "%s: the info lists no fields, or errors", what);
}

int main(int argc, char ** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    const std::string dsk = dir + "/test_agat140.dsk", hfe = dir + "/test_agat140.hfe";
    const BYTES disk = dos33_disk();
    write_all(dsk, disk);

    const DiskDefs defs;
    std::string format_id, type_id, fs_id;
    CHECK(detect_fdd_type(dsk, format_id, type_id, fs_id) && type_id == "TYPE_AGAT_140", "the .dsk is not a 140 Kb disk");
    auto image = prepare_image(dsk, format_id, type_id, defs);
    if (!image || !image->load()) {
        printf("FAIL: cannot load the .dsk\n");
        return 1;
    }

    auto writer = create_writer("FILE_HXC_HFE", 0xFE, image.get());
    BYTES file;
    CHECK(writer && writer->write(file), "writing the HFE");
    HfeImage img;
    CHECK(hfe_read(file, img), "reading the HFE back");
    printf("  %d tracks, %d sides, %u kbit/s, %s step\n", img.tracks, img.sides, img.bitrate,
           img.double_step ? "double" : "single");
    CHECK(img.tracks == 35 && img.sides == 1 && img.double_step && img.bitrate == 250,
          "not one side of 35 tracks, double step, 250 kbit/s");
    write_all(hfe, file);
    check_hfe(hfe, disk, "as written");

    // The transitions in the first cell of each pair, the encoding Apple II GCR
    for (BYTES & cells : img.cells) {
        BYTES first(cells.size(), 0);
        for (size_t c = 0; c + 1 < cells.size() * 8; c++)
            if ((cells[(c + 1) >> 3] >> ((c + 1) & 7)) & 1) first[c >> 3] |= (uint8_t)(1u << (c & 7));
        cells.swap(first);
    }
    img.encoding = APPLEII_GCR2_ENCODING;
    hfe_write(img, file);
    write_all(hfe, file);
    check_hfe(hfe, disk, "as HxC writes");

    printf(failures ? "%d FAILURES\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
