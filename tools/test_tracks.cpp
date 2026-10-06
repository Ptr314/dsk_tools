// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Tests of the whole track code (IBM MFM, IBM 3740 FM, DVK MX, HFE)
//
//   test_tracks [file.hfe ...]
//
// HFE files given on the command line must be told by hfe_probe_format() and
// read without a single bad sector (the eCat3 test results are such files)

#include <cstdio>
#include <cstring>
#include <chrono>
#include <fstream>
#include <iterator>

#include "dsk_tools/core.h"

using namespace dsk_tools;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

// Reproducible data, the same on every platform
static BYTES pattern(size_t n, uint32_t seed)
{
    BYTES b(n);
    uint32_t x = seed * 2654435761u + 1;
    for (auto & v : b) { x = x * 1103515245u + 12345u; v = (uint8_t)(x >> 16); }
    return b;
}

struct Digest {
    uint64_t h = 1469598103934665603ull;            // FNV-1a
    void add(const BYTES & b) { for (uint8_t v : b) { h ^= v; h *= 1099511628211ull; } }
};

static DiskFormatParams format_of(TrackLayout layout, unsigned heads, unsigned tracks, unsigned sectors, unsigned size,
                                  unsigned bitrate = 0, unsigned rpm = 0)
{
    DiskFormatParams f(heads, tracks, sectors, size, bitrate, rpm, 0, 0);
    f.layout = layout;
    track_layout_defaults(f);
    return f;
}

// The MY/MZ disks as diskdefs describe them
static DiskFormatParams rt11_mfm(unsigned heads)
{
    DiskFormatParams f = format_of(TrackLayout::IbmMfm, heads, 80, 10, 512);
    f.gaps.index_mark = false;
    f.gaps.gap1 = 42;
    f.gaps.gap3 = 36;
    return f;
}

// The tracks eCat3 writes and expects: digests of the code it was written
// with. A change here breaks the .hfe files of the emulator
static void test_ecat3_tracks()
{
    const DiskFormatParams my = rt11_mfm(2);
    const DiskFormatParams dx = format_of(TrackLayout::IbmFm, 1, 77, 26, 128);
    const DiskFormatParams mxf = format_of(TrackLayout::DvkMx, 2, 80, 11, 256);
    Digest mfm, fm, mx, c1, c2, c3;
    for (int t = 0; t < 5; t++) {
        BYTES d, s, cells;
        const uint16_t del = (uint16_t)(0x0123 * t);
        const BYTES f1 = pattern(10 * 512, t);
        ibm_mfm_format_track(d, s, t, t & 1, 10, 512, f1.data(), del); mfm.add(d); mfm.add(s);
        const BYTES f2 = pattern(26 * 128, 100 + t);
        ibm_fm_format_track(d, s, t, 0, 26, 128, f2.data(), del); fm.add(d); fm.add(s);
        const BYTES f3 = pattern(11 * 256, 200 + t);
        dvk_mx_format_track(d, t, 1, f3.data()); mx.add(d);
        CHECK(track_to_cells(my, t, 1, f1.data(), cells), "MY track"); c1.add(cells);
        CHECK(track_to_cells(dx, t, 0, f2.data(), cells), "DX track"); c2.add(cells);
        CHECK(track_to_cells(mxf, t, 1, f3.data(), cells), "MX track"); c3.add(cells);
    }
    CHECK(mfm.h == 0x85e8eeb467667fa8ull, "ibm_mfm_format_track changed");
    CHECK(fm.h  == 0x6c0e7b30c29b0654ull, "ibm_fm_format_track changed");
    CHECK(mx.h  == 0x43919ce5c3813ba2ull, "dvk_mx_format_track changed");
    CHECK(c1.h  == 0x97e34ae12e10f289ull, "MY/MZ cells changed");
    CHECK(c2.h  == 0x38eb1c66db7ab8f2ull, "DX cells changed");
    CHECK(c3.h  == 0x3b55a1cedf6187cbull, "MX cells changed");
}

static void test_roundtrip()
{
    const DiskFormatParams cases[] = {
        rt11_mfm(2),
        format_of(TrackLayout::IbmFm, 1, 77, 26, 128),
        format_of(TrackLayout::DvkMx, 2, 80, 11, 256),
    };
    for (const DiskFormatParams & f : cases)
        for (int t = 0; t < 5; t++) {
            const BYTES flat = pattern(f.sectors * f.sector_size, 300 + t);
            BYTES cells, out(flat.size());
            TrackStatus st;
            track_to_cells(f, t, 1, flat.data(), cells);
            CHECK(track_from_cells(f, cells, out.data(), st), "%s: read failed", track_layout_name(f.layout).c_str());
            CHECK(out == flat && st.missing == 0 && st.bad_crc == 0, "%s: missing %x bad %x",
                  track_layout_name(f.layout).c_str(), st.missing, st.bad_crc);
        }
}

// A standard track begins with the index mark C2 C2 C2 FC
static void test_index_mark()
{
    const DiskFormatParams f = format_of(TrackLayout::IbmMfm, 2, 80, 9, 512);
    const BYTES flat = pattern(9 * 512, 1);
    BYTES cells;
    CHECK(track_to_cells(f, 3, 0, flat.data(), cells), "9x512 does not fit");
    BYTES out(flat.size());
    TrackStatus st;
    CHECK(track_from_cells(f, cells, out.data(), st) && out == flat && st.missing == 0, "IAM track: missing %x", st.missing);
    // The byte level reader eCat3 uses
    BYTES data, special;
    mfm_decode(cells, cells.size() / 2, data, special);
    CHECK(data[80 + 12] == 0xC2 && data[80 + 15] == 0xFC, "no index mark at its place");
    uint16_t del;
    int found;
    CHECK(ibm_mfm_read_track(data.data(), data.size(), 9, 512, out.data(), del, found) && found == 9, "byte reader found %d", found);
}

// A broken data CRC, a broken ID and garbage between two sectors
static void test_damage()
{
    const DiskFormatParams f = format_of(TrackLayout::IbmMfm, 2, 80, 10, 512);
    const BYTES flat = pattern(10 * 512, 2);
    IbmTrackFormat fmt;
    fmt.sectors = 10;
    fmt.sector_size = 512;
    BYTES data, special, cells;
    CHECK(ibm_format_track(fmt, 5, 1, flat.data(), 1u << 7, data, special), "10x512 does not fit");
    IbmTrackFields fields;
    ibm_track_fields(data.data(), data.size(), nullptr, true, fields);
    CHECK(fields.sectors.size() == 10 && fields.index_marks.size() == 1, "fields %zu, index marks %zu",
          fields.sectors.size(), fields.index_marks.size());
    data[fields.sectors[2].data_pos + 100] ^= 0x55;
    data[fields.sectors[4].id_pos + 1] ^= 0x01;
    for (size_t p = fields.sectors[5].data_pos + 515; p < fields.sectors[6].id_pos - 16; p++) data[p] = (uint8_t)(p * 37);
    mfm_encode(data.data(), special.data(), data.size(), cells);
    BYTES out(flat.size(), 0);
    TrackStatus st;
    CHECK(track_from_cells(f, cells, out.data(), st), "damaged track not read");
    CHECK(st.bad_crc == (1u << 2), "bad_crc %x", st.bad_crc);
    CHECK(st.missing == (1u << 4), "missing %x", st.missing);
    CHECK(st.deleted == (1u << 7), "deleted %x", st.deleted);
    for (int s = 0; s < 10; s++)
        if (s != 2 && s != 4)
            CHECK(memcmp(out.data() + s * 512, flat.data() + s * 512, 512) == 0, "sector %d wrong", s + 1);
}

// A track written by a drive starts at any cell
static void test_phase()
{
    const DiskFormatParams f = format_of(TrackLayout::IbmMfm, 2, 80, 5, 1024);
    const BYTES flat = pattern(5 * 1024, 3);
    BYTES cells;
    CHECK(track_to_cells(f, 0, 0, flat.data(), cells), "5x1024 does not fit");
    const int shifts[] = {1, 3, 7, 11};
    for (int shift : shifts) {
        BYTES rot(cells.size(), 0);
        const size_t bits = cells.size() * 8;
        for (size_t b = 0; b < bits; b++) {
            const size_t from = (b + shift) % bits;
            if ((cells[from >> 3] >> (from & 7)) & 1) rot[b >> 3] |= (uint8_t)(1u << (b & 7));
        }
        BYTES out(flat.size());
        TrackStatus st;
        track_from_cells(f, rot, out.data(), st);
        CHECK(st.missing == 0 && st.bad_crc == 0 && out == flat, "shift %d: missing %x bad %x", shift, st.missing, st.bad_crc);
    }
}

// Whole disks of every geometry through the HFE container and the probe
static void test_geometries()
{
    struct { const char * name; DiskFormatParams f; } g[] = {
        {"pc-360",  format_of(TrackLayout::IbmMfm, 2, 40, 9, 512)},
        {"pc-1200", format_of(TrackLayout::IbmMfm, 2, 80, 15, 512, 500, 360)},
        {"pc-1440", format_of(TrackLayout::IbmMfm, 2, 80, 18, 512, 500)},
        {"st-800",  format_of(TrackLayout::IbmMfm, 2, 80, 10, 512)},
        {"korvet",  format_of(TrackLayout::IbmMfm, 2, 80, 5, 1024)},
        {"dx",      format_of(TrackLayout::IbmFm, 1, 77, 26, 128)},
        {"mx-220",  format_of(TrackLayout::DvkMx, 2, 40, 11, 256)},
    };
    for (auto & x : g) {
        const DiskFormatParams & f = x.f;
        HfeImage img;
        track_hfe_params(f, img);
        const BYTES disk = pattern((size_t)f.heads * f.tracks * f.sectors * f.sector_size, 4);
        const size_t ts = (size_t)f.sectors * f.sector_size;
        bool fits = true;
        for (unsigned t = 0; t < f.tracks; t++)
            for (unsigned h = 0; h < f.heads; h++)
                fits &= track_to_cells(f, t, h, disk.data() + (t * f.heads + h) * ts, img.cells[t * f.heads + h]);
        CHECK(fits, "%s does not fit", x.name);
        BYTES file;
        hfe_write(img, file);
        HfeImage back;
        CHECK(hfe_read(file, back), "%s: hfe_read", x.name);
        DiskFormatParams probe;
        CHECK(hfe_probe_format(back, probe), "%s: not probed", x.name);
        CHECK(probe.layout == f.layout && probe.sectors == f.sectors && probe.sector_size == f.sector_size
              && probe.heads == f.heads && probe.tracks == f.tracks && probe.bitrate == f.bitrate,
              "%s: probed as %s %ux%u, %u heads, %u tracks, %u kbit/s", x.name, track_layout_name(probe.layout).c_str(),
              probe.sectors, probe.sector_size, probe.heads, probe.tracks, probe.bitrate);
        BYTES flat;
        BadSectorTable bad;
        hfe_to_flat(back, f, flat, &bad);
        CHECK(flat == disk && bad.empty(), "%s: disk differs, %zu bad sectors", x.name, bad.size());
        printf("  %-8s %5d bytes a turn\n", x.name, track_bytes(f));
    }
}

static void test_files(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++) {
        std::ifstream in(argv[i], std::ios::binary);
        const BYTES file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        HfeImage img;
        DiskFormatParams f;
        if (!hfe_read(file, img) || !hfe_probe_format(img, f)) {
            CHECK(false, "%s: not an IBM or MX disk", argv[i]);
            continue;
        }
        // RT-11 MY/MZ tracks have no index mark; reading does not care
        BYTES flat;
        BadSectorTable bad;
        const auto t0 = std::chrono::steady_clock::now();
        hfe_to_flat(img, f, flat, &bad);
        const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        printf("  %s: %s %ux%u, %u heads, %u tracks, %lld ms, %zu bad\n", argv[i], track_layout_name(f.layout).c_str(),
               f.sectors, f.sector_size, f.heads, f.tracks, ms, bad.size());
        CHECK(bad.empty(), "%s: %zu bad sectors", argv[i], bad.size());
    }
}

int main(int argc, char ** argv)
{
    printf("eCat3 tracks\n");   test_ecat3_tracks();
    printf("round trip\n");     test_roundtrip();
    printf("index mark\n");     test_index_mark();
    printf("damage\n");         test_damage();
    printf("phase\n");          test_phase();
    printf("geometries\n");     test_geometries();
    if (argc > 1) { printf("files\n"); test_files(argc, argv); }
    printf(failures ? "%d FAILURES\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
