// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mike Shevchenko <mike.shevchenko@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Apple II and Agat 140 Kb disks through the public API
//
//   test_agat140 [scratch directory]
//
// A DOS 3.3 disk made up here is written to an HFE, which goes through
// detect_fdd_type(), prepare_image(), the filesystem and file_info() as the GUI
// uses them; then again with the transitions in the first cell of each pair and
// the encoding named, as the HxC software writes such files. That disk and a
// made-up ProDOS volume then go through sector images in both orders

#include <cstdio>
#include <cstring>
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
// on the disk but the first byte 01 a boot sector starts with. The last track
// holds a pattern, for disks of more than 35 tracks to show it kept
static BYTES dos33_disk(int tracks = 35)
{
    BYTES disk(tracks * 16 * 256, 0);
    auto sector = [&](int t, int s) { return disk.data() + (t * 16 + s) * 256; };
    disk[0] = 0x01;
    // The VTOC: the catalog's first track and sector, the DOS release, the volume,
    // track/sector pairs in a list, the track allocated last and the way onward,
    // tracks, sectors a track and 256 bytes a sector
    uint8_t * vtoc = sector(17, 0);
    vtoc[0x01] = 17;
    vtoc[0x02] = 15;
    vtoc[0x03] = 3;
    vtoc[0x06] = 0xFE;
    vtoc[0x27] = 122;
    vtoc[0x30] = 18;
    vtoc[0x31] = 1;
    vtoc[0x34] = (uint8_t)tracks;
    vtoc[0x35] = 16;
    vtoc[0x37] = 1;
    for (int t = 0; t < tracks; t++) {
        // A bit for each free sector, 15 to 0: the catalog takes track 17, the file two
        // sectors, the pattern the last track
        const uint16_t free = (t == 17 || t == tracks - 1) ? 0x0000 : (t == 18) ? 0x3FFF : 0xFFFF;
        vtoc[0x38 + 4 * t] = (uint8_t)(free >> 8);
        vtoc[0x39 + 4 * t] = (uint8_t)free;
    }
    for (int s = 15; s > 1; s--) {
        sector(17, s)[1] = 17;
        sector(17, s)[2] = (uint8_t)(s - 1);
    }
    // The file's entry: its track/sector list, Applesoft, its name, two sectors
    uint8_t * entry = sector(17, 15) + 0x0B;
    entry[0] = 18;
    entry[1] = 15;
    entry[2] = 0x02;
    const char name[] = "HELLO";
    for (int i = 0; i < 30; i++) entry[3 + i] = (uint8_t)((i < 5 ? name[i] : ' ') | 0x80);
    entry[0x21] = 2;
    for (int i = 0; i < 16 * 256; i++) sector(tracks - 1, 0)[i] = (uint8_t)(i * 37 + tracks);
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

// The file's sector for each physical sector of a track, in DOS 3.3 order (.dsk,
// .do) and in ProDOS order (.po), as the formats have them
static const int dos_order[16] = {0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15};
static const int prodos_order[16] = {0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15};

// An image with its sectors moved from one order to the other
static BYTES reordered(const BYTES & in, const int * from, const int * to)
{
    BYTES out(in.size());
    for (size_t t = 0; t + 16 * 256 <= in.size(); t += 16 * 256)
        for (int p = 0; p < 16; p++)
            std::memcpy(out.data() + t + to[p] * 256, in.data() + t + from[p] * 256, 256);
    return out;
}

// A ProDOS volume of 280 blocks in ProDOS order, as a .po holds it: block b is
// sectors 2(b % 8) and 2(b % 8) + 1 of track b / 8. The volume directory in
// blocks 2 to 5, the bitmap in block 6 and one file, a seedling, in block 7
static BYTES prodos_volume()
{
    BYTES po(280 * 512, 0);
    auto block = [&](int b) { return po.data() + b * 512; };
    // Each block of the directory names the blocks before and after it
    for (int b = 2; b <= 5; b++) {
        block(b)[0] = (uint8_t)(b == 2 ? 0 : b - 1);
        block(b)[2] = (uint8_t)(b == 5 ? 0 : b + 1);
    }
    // The volume directory header: name, 39 bytes an entry, 13 entries a block, one
    // file, the bitmap's block, the blocks of the volume
    uint8_t * header = block(2) + 4;
    header[0x00] = 0xF0 | 4;
    std::memcpy(header + 0x01, "TEST", 4);
    header[0x1F] = 0x27;
    header[0x20] = 0x0D;
    header[0x21] = 1;
    header[0x23] = 6;
    header[0x25] = 280 & 0xFF;
    header[0x26] = 280 >> 8;
    // The file: a seedling, its name, type TXT, its block, one block used, 6 bytes, and
    // the directory it is in
    uint8_t * entry = header + 0x27;
    entry[0x00] = 0x10 | 9;
    std::memcpy(entry + 0x01, "HELLO.TXT", 9);
    entry[0x10] = 0x04;
    entry[0x11] = 7;
    entry[0x13] = 1;
    entry[0x15] = 6;
    entry[0x25] = 2;
    for (int b = 8; b < 280; b++) block(6)[b / 8] |= (uint8_t)(0x80 >> (b % 8));
    std::memcpy(block(7), "HELLO", 5);
    block(7)[5] = 0x0D;
    return po;
}

// Opens an image as the GUI does: its type and file system, the disk held in DOS 3.3
// order, and one file listed
static std::unique_ptr<diskImage> open_image(const std::string & name, const char * fs_expected,
    const char * file, const BYTES & held, const char * what)
{
    const DiskDefs defs;
    std::string format_id, type_id, fs_id;
    CHECK(detect_fdd_type(name, format_id, type_id, fs_id) && type_id == "TYPE_AGAT_140" && fs_id == fs_expected,
          "%s: detected as %s, %s", what, type_id.c_str(), fs_id.c_str());
    auto image = prepare_image(name, format_id, type_id, defs);
    if (!image || !image->load()) {
        CHECK(false, "%s: does not load", what);
        return nullptr;
    }
    CHECK(*image->get_buffer() == held, "%s: not held in DOS 3.3 order", what);
    auto fs = prepare_filesystem(image.get(), fs_expected, defs);
    std::vector<UniversalFile> files;
    CHECK(fs && fs->open() && fs->dir(files, false), "%s: the catalog does not open", what);
    CHECK(files.size() == 1 && files[0].name == file, "%s: %zu files, not %s", what, files.size(), file);
    return image;
}

// A RAW writer's output for the image, into a file of the name given
static BYTES written(diskImage * image, const std::string & name)
{
    auto writer = create_writer("FILE_RAW_MSB", 0xFE, image, name);
    BYTES out;
    CHECK(writer && writer->write(out), "writing %s", name.c_str());
    return out;
}

static void test_orders(const std::string & dir, const BYTES & dos33)
{
    printf("sector orders\n");
    // The DOS 3.3 disk as a .po
    const BYTES dos33_po = reordered(dos33, dos_order, prodos_order);
    const std::string po = dir + "/test_order.po";
    write_all(po, dos33_po);
    auto image = open_image(po, "FILESYSTEM_DOS33", "HELLO", dos33, "DOS 3.3 as .po");
    if (image) {
        CHECK(written(image.get(), "") == dos33_po, ".po saved back in ProDOS order");
        CHECK(written(image.get(), "x.po") == dos33_po, ".po to .po");
        CHECK(written(image.get(), "x.dsk") == dos33, ".po to .dsk");
        CHECK(written(image.get(), "x.do") == dos33, ".po to .do");
        auto nic = create_writer("FILE_MFM_NIC", 0xFE, image.get());
        BYTES from_po, from_dsk;
        CHECK(nic && nic->write(from_po), ".po to .nic");
        write_all(dir + "/test_order.dsk", dos33);
        const DiskDefs defs;
        auto dsk = prepare_image(dir + "/test_order.dsk", "FILE_RAW_MSB", "TYPE_AGAT_140", defs);
        auto nic2 = dsk && dsk->load() ? create_writer("FILE_MFM_NIC", 0xFE, dsk.get()) : nullptr;
        CHECK(nic2 && nic2->write(from_dsk) && from_po == from_dsk, ".po and .dsk give other .nic");
    }

    // The DOS 3.3 disk in the order its name does not say: the catalog tells
    struct DosCase { const char * name; const BYTES * data; bool prodos; };
    const DosCase dos_cases[] = {
        {"/test_order_do.po", &dos33, false},
        {"/test_order_po.dsk", &dos33_po, true},
        {"/test_order_po.do", &dos33_po, true},
    };
    for (const DosCase & c : dos_cases) {
        const std::string name = dir + c.name;
        write_all(name, *c.data);
        auto dos = open_image(name, "FILESYSTEM_DOS33", "HELLO", dos33, c.name);
        if (!dos) continue;
        CHECK(dos->file_prodos_order() == c.prodos, "%s: file order", c.name);
        CHECK(written(dos.get(), "") == *c.data, "%s: not saved back in its order", c.name);
    }

    // The ProDOS volume in both orders under each name
    const BYTES pv_po = prodos_volume();
    const BYTES pv_dos = reordered(pv_po, prodos_order, dos_order);
    struct Case { const char * name; const BYTES * data; bool prodos; };
    const Case cases[] = {
        {"/test_prodos.po", &pv_po, true},
        {"/test_prodos_po.dsk", &pv_po, true},
        {"/test_prodos_po.do", &pv_po, true},
        {"/test_prodos.do", &pv_dos, false},
        {"/test_prodos_do.dsk", &pv_dos, false},
        {"/test_prodos_do.po", &pv_dos, false},
    };
    for (const Case & c : cases) {
        const std::string name = dir + c.name;
        write_all(name, *c.data);
        auto pv = open_image(name, "FILESYSTEM_PRODOS", "HELLO.TXT", pv_dos, c.name);
        if (!pv) continue;
        CHECK(pv->file_prodos_order() == c.prodos, "%s: file order", c.name);
        CHECK(written(pv.get(), "") == *c.data, "%s: not saved back in its order", c.name);
        const DiskDefs defs;
        auto fs = prepare_filesystem(pv.get(), "FILESYSTEM_PRODOS", defs);
        std::vector<UniversalFile> files;
        BYTES text;
        CHECK(fs && fs->open() && fs->dir(files, false) && files.size() == 1
              && fs->get_file(files[0], "", text) && text == BYTES({'H', 'E', 'L', 'L', 'O', 0x0D}),
              "%s: HELLO.TXT does not read back", c.name);
    }

    // The ProDOS volume in track images, which hold the sectors where the disk has them
    const DiskDefs defs;
    auto pv = prepare_image(dir + "/test_prodos.po", "FILE_RAW_MSB", "TYPE_AGAT_140", defs);
    const bool pv_loaded = pv && pv->load();
    CHECK(pv_loaded, "ProDOS .po does not load");
    const char * track_images[][2] = {
        {"FILE_MFM_NIC", "/test_prodos.nic"},
        {"FILE_MFM_NIB", "/test_prodos.nib"},
        {"FILE_HXC_HFE", "/test_prodos.hfe"},
    };
    for (const auto & t : track_images) {
        if (!pv_loaded) break;
        auto writer = create_writer(t[0], 0xFE, pv.get());
        BYTES data;
        CHECK(writer && writer->write(data), "ProDOS to %s", t[1]);
        write_all(dir + t[1], data);
        open_image(dir + t[1], "FILESYSTEM_PRODOS", "HELLO.TXT", pv_dos, t[1]);
    }
}

// A disk of 40 tracks through every image format, with the 128-byte label of some
// Agat images, and a file of more tracks than such disks have
static void test_forty_tracks(const std::string & dir)
{
    printf("40 tracks\n");
    const BYTES disk = dos33_disk(40);
    const std::string dsk = dir + "/test_forty.dsk";
    write_all(dsk, disk);
    auto image = open_image(dsk, "FILESYSTEM_DOS33", "HELLO", disk, "40 tracks as .dsk");
    if (!image) return;
    CHECK(image->get_tracks() == 40, ".dsk: %u tracks", image->get_tracks());
    const char * formats[][2] = {
        {"FILE_RAW_MSB", "/test_forty.po"},
        {"FILE_MFM_NIC", "/test_forty.nic"},
        {"FILE_MFM_NIB", "/test_forty.nib"},
        {"FILE_HXC_MFM", "/test_forty.mfm"},
        {"FILE_HXC_HFE", "/test_forty.hfe"},
    };
    for (const auto & f : formats) {
        const std::string name = dir + f[1];
        auto writer = create_writer(f[0], 0xFE, image.get(), name);
        BYTES data;
        CHECK(writer && writer->write(data), "40 tracks to %s", f[1]);
        write_all(name, data);
        auto again = open_image(name, "FILESYSTEM_DOS33", "HELLO", disk, f[1]);
        CHECK(again && again->get_tracks() == 40, "%s: not 40 tracks", f[1]);
    }

    BYTES labelled(disk);
    labelled.resize(disk.size() + 128, 0);
    std::memcpy(labelled.data() + disk.size(), "AGAT-DISK", 9);
    write_all(dir + "/test_forty_label.dsk", labelled);
    open_image(dir + "/test_forty_label.dsk", "FILESYSTEM_DOS33", "HELLO", disk, "40 tracks and a label");

    write_all(dir + "/test_41.dsk", dos33_disk(41));
    std::string format_id, type_id, fs_id;
    CHECK(!detect_fdd_type(dir + "/test_41.dsk", format_id, type_id, fs_id) || type_id != "TYPE_AGAT_140",
        "41 tracks taken for a 140 Kb disk");

    // Track images cut short are refused, not read past their end
    const char * cut[][2] = {
        {"FILE_MFM_NIC", "/test_cut.nic"},
        {"FILE_MFM_NIB", "/test_cut.nib"},
        {"FILE_HXC_MFM", "/test_cut.mfm"},
    };
    for (const auto & f : cut) {
        auto writer = create_writer(f[0], 0xFE, image.get());
        BYTES data;
        CHECK(writer && writer->write(data), "40 tracks to %s", f[1]);
        data.resize(data.size() / 4);
        write_all(dir + f[1], data);
        const DiskDefs defs;
        auto cut_image = prepare_image(dir + f[1], f[0], "TYPE_AGAT_140", defs);
        CHECK(!cut_image || !cut_image->load(), "%s: a cut file loads", f[1]);
    }
}

// What the VTOC of a disk made by dos33_disk() says of a track: a bit for each free sector, 15 to 0
static void set_free(BYTES & disk, int track, uint16_t free)
{
    uint8_t * vtoc = disk.data() + 17 * 16 * 256;
    vtoc[0x38 + 4 * track] = (uint8_t)(free >> 8);
    vtoc[0x39 + 4 * track] = (uint8_t)free;
}

// Adds a file of 256 bytes, which takes two sectors with its T/S list. Tells whether
// the filesystem took it, and checks that it then reads back and HELLO is still there
static bool put_small_file(const std::string & name, const BYTES & disk, const char * what)
{
    write_all(name, disk);
    auto image = open_image(name, "FILESYSTEM_DOS33", "HELLO", disk, what);
    if (!image) return false;
    const DiskDefs defs;
    auto fs = prepare_filesystem(image.get(), "FILESYSTEM_DOS33", defs);
    Files files;
    CHECK(fs && fs->open(), "%s: does not open", what);

    UniversalFile uf{};
    uf.fs = FS::Host;
    uf.name = "NEW";
    const BYTES data(256, 0x5A);
    const bool taken = static_cast<bool>(fs->put_file(uf, "FILE_ANY", data, false));

    CHECK(fs->dir(files, false), "%s: the catalog does not read", what);
    CHECK(files.size() == (taken ? 2u : 1u), "%s: %zu files, taken: %d", what, files.size(), taken);
    if (taken && files.size() == 2) {
        BYTES back;
        CHECK(fs->get_file(files[1], "", back) && back == data, "%s: the file does not read back", what);
    }
    return taken;
}

// Sectors are only taken from what the filesystem can address and owns
static void test_allocation(const std::string & dir)
{
    printf("allocation\n");
    const std::string name = dir + "/test_alloc.dsk";

    // Track 0 free in the VTOC: 0 there would end the catalog and T/S lists
    BYTES disk = dos33_disk();
    for (int t = 1; t < 35; t++) set_free(disk, t, 0);
    CHECK(!put_small_file(name, disk, "only track 0 free"), "a file taken on track 0");

    // Below the catalog, the two sectors track 1 has are enough, track 0 is never touched
    set_free(disk, 1, 0x0003);
    CHECK(put_small_file(name, disk, "two sectors on track 1"), "the sectors of track 1 not used");

    // Past the tracks the VTOC declares is not the filesystem's, whatever the bitmap says
    BYTES forty = dos33_disk(40);
    for (int t = 0; t < 40; t++) set_free(forty, t, t >= 35 ? 0xFFFF : 0);
    forty[17 * 16 * 256 + 0x34] = 35;
    CHECK(!put_small_file(name, forty, "tracks past the VTOC"), "a file taken past the tracks of the VTOC");
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

    test_orders(dir, disk);
    test_forty_tracks(dir);
    test_allocation(dir);

    // An Agat 840 Kb disk, whose image class derives from the 140 Kb one, keeps its
    // 2 x 80 tracks of 21 sectors
    printf("840 Kb\n");
    const std::string dsk840 = dir + "/test_agat840.dsk";
    write_all(dsk840, BYTES(2 * 80 * 21 * 256, 0));
    auto image840 = prepare_image(dsk840, "FILE_RAW_MSB", "TYPE_AGAT_840", defs);
    CHECK(image840 && image840->load() && image840->get_tracks() == 80, "an 840 Kb image does not load");

    printf(failures ? "%d FAILURES\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
