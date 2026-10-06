// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: The HFE loader and writer through the public API, with a damaged track
//
//   test_hfe_io <image> <type_id> [scratch directory]
//
// The raw image is written to an HFE, two sectors of it are damaged (a data
// bit of the third sector of track 2, the ID of the fifth sector of track 3),
// and the file goes through detect_fdd_type(), prepare_image(), the
// filesystem, load_structured() and file_info() as the GUI uses them

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>

#include "dsk_tools/dsk_tools.h"

using namespace dsk_tools;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static BYTES read_all(const std::string & name)
{
    std::ifstream in(name, std::ios::binary);
    return BYTES((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// The logical sector whose place on the track is this slot
static unsigned logical_of(const DiskFormatParams & f, unsigned slot)
{
    if (f.sector_translation.empty()) return slot;
    return static_cast<unsigned>(std::find(f.sector_translation.begin(), f.sector_translation.end(), slot) - f.sector_translation.begin());
}

int main(int argc, char ** argv)
{
    if (argc < 3) {
        printf("usage: test_hfe_io <image> <type_id> [scratch directory]\n");
        return 2;
    }
    const std::string raw = argv[1], type = argv[2];
    const std::string name = std::string(argc > 3 ? argv[3] : ".") + "/test_hfe_io.hfe";
    const BYTES defs_file = read_all(DSK_TOOLS_DISKDEFS);
    const DiskDefs defs = parse_diskdefs(std::string(defs_file.begin(), defs_file.end()));

    std::string format_id, type_id, fs_id;
    CHECK(detect_fdd_type(raw, format_id, type_id, fs_id, true), "%s: unknown format", raw.c_str());
    auto image = prepare_image(raw, format_id, type, defs);
    if (!image || !image->load()) {
        printf("FAIL: cannot load %s as %s\n", raw.c_str(), type.c_str());
        return 1;
    }
    const DiskFormatParams & f = image->get_format();
    CHECK(f.layout != TrackLayout::None, "%s has no track layout", type.c_str());

    auto writer = create_writer("FILE_HXC_HFE", 0xFE, image.get());
    BYTES hfe;
    CHECK(writer && writer->write(hfe), "writing the HFE");
    HfeImage img;
    CHECK(hfe_read(hfe, img), "reading the HFE back");
    printf("  %d tracks, %d sides, %u kbit/s, %u rpm, %s\n", img.tracks, img.sides, img.bitrate, img.rpm,
           track_layout_name(f.layout).c_str());

    // One data cell flipped: 16 cells a byte, the data cells are the odd ones
    const bool ibm = f.layout != TrackLayout::DvkMx;
    auto damage = [&](int t, int h, int sector, bool id) {
        BYTES & cells = img.cells[t * img.sides + h];
        BYTES data;
        IbmTrackFields fields;
        track_decode(f, cells, data, fields);
        const IbmSectorField & s = fields.sectors[sector];
        const size_t bit = (id ? s.id_pos + 2 : s.data_pos + 40) * 16 + 3;
        cells[bit >> 3] ^= (uint8_t)(1u << (bit & 7));
    };
    if (ibm) {
        damage(2, img.sides - 1, 2, false);
        damage(3, 0, 4, true);
    }
    hfe_write(img, hfe);
    {
        std::ofstream out(name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(hfe.data()), hfe.size());
    }

    CHECK(detect_fdd_type(name, format_id, type_id, fs_id), "detecting the HFE");
    printf("  detected %s, %s\n", type_id.c_str(), fs_id.c_str());
    CHECK(type_id == type, "detected as %s", type_id.c_str());
    auto back = prepare_image(name, format_id, type, defs);
    if (!back || !back->load()) {
        printf("FAIL: cannot load the HFE\n");
        return 1;
    }
    if (ibm) {
        CHECK(back->has_bad_sectors(), "no bad sectors");
        CHECK(back->is_bad_sector(img.sides - 1, 2, logical_of(f, 2)), "track 2 sector 3 not bad");
        CHECK(back->is_bad_sector(0, 3, logical_of(f, 4)), "track 3 sector 5 not bad");
        CHECK(!back->is_bad_sector(0, 3, logical_of(f, 3)), "track 3 sector 4 bad");
    } else {
        CHECK(!back->has_bad_sectors() && *back->get_buffer() == *image->get_buffer(), "MX disk differs");
    }

    std::string fs_name = fs_id;
    auto fs = prepare_filesystem(back.get(), fs_id, defs);
    if (fs && fs->open()) {
        std::vector<UniversalFile> files;
        fs->dir(files, false);
        printf("  %s: %zu files\n", fs_name.c_str(), files.size());
    } else
        printf("  %s does not open (a blank disk?)\n", fs_name.c_str());

    StructDisk disk;
    CHECK(back->load_structured(disk), "load_structured");
    size_t sectors = 0, bad = 0;
    for (const StructTrack & t : disk.tracks)
        for (const StructSector & s : t.sectors) { sectors++; bad += s.is_bad; }
    const size_t expected = (size_t)f.tracks * f.heads * f.sectors - (ibm ? 1 : 0);
    printf("  structured: %zu tracks, %zu sectors, %zu bad\n", disk.tracks.size(), sectors, bad);
    CHECK(sectors == expected && bad == (ibm ? 1u : 0u), "expected %zu sectors, one bad", expected);

    const std::string info = back->file_info();
    CHECK(info.find("{$TRACK_LAYOUT}: " + track_layout_name(f.layout)) != std::string::npos, "no layout in the info");
    if (ibm) CHECK(info.find("{$INDEX_CRC_ERROR}") != std::string::npos && info.find("{$SECTOR_CRC_ERROR}") != std::string::npos,
                   "the damage is not in the info");

    printf(failures ? "%d FAILURES\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
