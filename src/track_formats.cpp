// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Whole track layouts of the IBM MFM, IBM 3740 FM and DVK MX disks

#include <cstring>
#include <utility>

#include "dsk_tools/core.h"

namespace dsk_tools {

namespace {

    uint16_t ibm_crc(uint16_t crc, const uint8_t * p, size_t n)
    {
        for (size_t k = 0; k < n; k++) {
            crc ^= (uint16_t)(p[k] << 8);
            for (int i = 0; i < 8; i++)
                crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
        return crc;
    }

    const int     MFM_GAP_FIRST   = 42;     // GAP4a + GAP1 before the first sector
    const int     MFM_GAP_SECTOR  = 36;     // GAP3 between sectors
    const int     MFM_GAP_HEADER  = 22;     // GAP2 between the header and the data
    const int     MFM_SYNC_BYTES  = 12;
    const uint8_t MFM_GAP_BYTE    = 0x4E;
    const uint8_t MFM_MARK_BYTE   = 0xA1;

    const int     FM_GAP4A        = 40;
    const int     FM_GAP1         = 26;
    const int     FM_GAP2         = 11;
    const int     FM_GAP3         = 27;
    const int     FM_SYNC         = 6;
    const uint8_t FM_GAP_BYTE     = 0xFF;

    const uint8_t MARK_INDEX      = 0xFC;
    const uint8_t MARK_ID         = 0xFE;
    const uint8_t MARK_DATA       = 0xFB;
    const uint8_t MARK_DELETED    = 0xF8;

    // Walks the fields of an FM track: a mark is FC/FE/FB/F8 after zeros;
    // an ID field is followed by 6 bytes, a data field by the sector of the
    // size the last ID named and its CRC
    template <typename F>
    void fm_walk(const uint8_t * data, size_t len, F field)
    {
        size_t p = 0;
        int size = 128;
        while (p < len) {
            if (data[p] != 0) { p++; continue; }
            while (p < len && data[p] == 0) p++;
            if (p >= len) break;
            const uint8_t m = data[p];
            if (m == MARK_INDEX) { field((int)p, m, 0); p++; continue; }
            if (m == MARK_ID) {
                if (p + 7 > len) break;
                const uint8_t code = data[p + 4];
                size = (code <= 6) ? (128 << code) : 128;
                field((int)p, m, 0);
                p += 7;
                continue;
            }
            if (m == MARK_DATA || m == MARK_DELETED) {
                field((int)p, m, size);
                p += 1 + (size_t)size + 2;
                continue;
            }
            p++;
        }
    }

}

int ibm_size_code(int sector_size)
{
    int code = 0;
    while ((128 << code) < sector_size && code < 6) code++;
    return code;
}

//----------------------------- IBM MFM ------------------------------------//

void ibm_mfm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                          const uint8_t * flat, uint16_t deleted)
{
    data.assign(IBM_MFM_TRACK_BYTES, MFM_GAP_BYTE);
    special.assign(IBM_MFM_TRACK_BYTES, 0);
    size_t p = 0;
    int gap = MFM_GAP_FIRST;
    for (int sect = 0; sect < sectors; sect++) {
        p += gap;
        memset(&data[p], 0, MFM_SYNC_BYTES); p += MFM_SYNC_BYTES;

        size_t start = p;
        for (int i = 0; i < 3; i++) { special[p] = 1; data[p++] = MFM_MARK_BYTE; }
        data[p++] = MARK_ID;
        data[p++] = (uint8_t)track;
        data[p++] = (uint8_t)side;
        data[p++] = (uint8_t)(sect + 1);
        data[p++] = (uint8_t)ibm_size_code(sector_size);
        uint16_t crc = ibm_crc(0xFFFF, &data[start], p - start);
        data[p++] = (uint8_t)(crc >> 8);
        data[p++] = (uint8_t)(crc & 0xFF);

        p += MFM_GAP_HEADER;
        memset(&data[p], 0, MFM_SYNC_BYTES); p += MFM_SYNC_BYTES;

        start = p;
        for (int i = 0; i < 3; i++) { special[p] = 1; data[p++] = MFM_MARK_BYTE; }
        data[p++] = (deleted & (1u << sect)) ? MARK_DELETED : MARK_DATA;
        if (flat != nullptr) memcpy(&data[p], flat + (size_t)sect * sector_size, sector_size);
        else memset(&data[p], 0, sector_size);
        p += sector_size;
        crc = ibm_crc(0xFFFF, &data[start], p - start);
        data[p++] = (uint8_t)(crc >> 8);
        data[p++] = (uint8_t)(crc & 0xFF);

        gap = MFM_GAP_SECTOR;
    }
}

bool ibm_mfm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                        uint16_t & deleted, int & found)
{
    size_t p = 0;
    found = 0;
    deleted = 0;
    for (;;) {
        while (p < len && data[p] == MFM_GAP_BYTE) p++;
        if (p >= len) break;                                    // end of the track
        while (p < len && data[p] == 0) p++;
        if (p >= len) return false;
        for (int i = 0; i < 3 && p < len && data[p] == MFM_MARK_BYTE; i++) p++;
        if (p >= len || data[p++] != MARK_ID) return false;
        if (p + 6 > len) return false;
        const int id_sector = data[p + 2];
        const uint8_t size_code = data[p + 3];
        p += 4 + 2;                                             // the ID fields and CRC
        if (size_code > 6) return false;
        const size_t size = (size_t)128 << size_code;

        while (p < len && data[p] == MFM_GAP_BYTE) p++;
        while (p < len && data[p] == 0) p++;
        for (int i = 0; i < 3 && p < len && data[p] == MFM_MARK_BYTE; i++) p++;
        if (p >= len) return false;
        const uint8_t mark = data[p++];
        if (mark != MARK_DATA && mark != MARK_DELETED) return false;
        if (p + size + 2 > len) return false;
        // Placed by the number in its ID field
        if (id_sector >= 1 && id_sector <= sectors && (int)size == sector_size) {
            memcpy(out + (size_t)(id_sector - 1) * sector_size, data + p, size);
            if (mark == MARK_DELETED) deleted |= (uint16_t)(1u << (id_sector - 1));
            found++;
        }
        p += size + 2;                                          // data and CRC
    }
    return true;
}

// Sync marks by the bytes alone: three A1 after zeros and before FE, FB or F8
void ibm_mfm_find_marks(const uint8_t * data, size_t len, BYTES & special)
{
    special.assign(len, 0);
    for (size_t p = 1; p + 3 < len; p++)
        if (data[p - 1] == 0 && data[p] == MFM_MARK_BYTE && data[p + 1] == MFM_MARK_BYTE && data[p + 2] == MFM_MARK_BYTE
            && (data[p + 3] == MARK_ID || data[p + 3] == MARK_DATA || data[p + 3] == MARK_DELETED))
            special[p] = special[p + 1] = special[p + 2] = 1;
}

//----------------------------- IBM 3740 FM --------------------------------//

void ibm_fm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                         const uint8_t * flat, uint16_t deleted)
{
    data.assign(IBM_FM_TRACK_BYTES, FM_GAP_BYTE);
    special.assign(IBM_FM_TRACK_BYTES, 0);
    size_t p = FM_GAP4A;
    memset(&data[p], 0, FM_SYNC); p += FM_SYNC;
    special[p] = 1;
    data[p++] = MARK_INDEX;
    p += FM_GAP1;
    for (int sect = 0; sect < sectors; sect++) {
        memset(&data[p], 0, FM_SYNC); p += FM_SYNC;
        special[p] = 1;
        size_t start = p;
        data[p++] = MARK_ID;
        data[p++] = (uint8_t)track;
        data[p++] = (uint8_t)side;
        data[p++] = (uint8_t)(sect + 1);
        data[p++] = (uint8_t)ibm_size_code(sector_size);
        uint16_t crc = ibm_crc(0xFFFF, &data[start], p - start);
        data[p++] = (uint8_t)(crc >> 8);
        data[p++] = (uint8_t)(crc & 0xFF);
        p += FM_GAP2;
        memset(&data[p], 0, FM_SYNC); p += FM_SYNC;
        special[p] = 1;
        start = p;
        data[p++] = (deleted & (1u << sect)) ? MARK_DELETED : MARK_DATA;
        if (flat != nullptr) memcpy(&data[p], flat + (size_t)sect * sector_size, sector_size);
        else memset(&data[p], 0, sector_size);
        p += sector_size;
        crc = ibm_crc(0xFFFF, &data[start], p - start);
        data[p++] = (uint8_t)(crc >> 8);
        data[p++] = (uint8_t)(crc & 0xFF);
        p += FM_GAP3;
    }
}

bool ibm_fm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                       uint16_t & deleted, int & found)
{
    found = 0;
    deleted = 0;
    int id_sector = -1;
    fm_walk(data, len, [&](int p, uint8_t m, int size) {
        if (m == MARK_ID) { id_sector = data[p + 3]; return; }
        if ((m == MARK_DATA || m == MARK_DELETED) && id_sector >= 1 && id_sector <= sectors
            && size == sector_size && (size_t)p + 1 + size <= len) {
            memcpy(out + (size_t)(id_sector - 1) * sector_size, data + p + 1, size);
            if (m == MARK_DELETED) deleted |= (uint16_t)(1u << (id_sector - 1));
            found++;
        }
        id_sector = -1;
    });
    return found > 0;
}

void ibm_fm_find_marks(const uint8_t * data, size_t len, BYTES & special)
{
    special.assign(len, 0);
    fm_walk(data, len, [&](int p, uint8_t, int) { special[(size_t)p] = 1; });
}

int ibm_fm_find_sector(const uint8_t * data, size_t len, int track, int sector, bool & deleted, int & size)
{
    int found = -1;
    bool want = false;
    fm_walk(data, len, [&](int p, uint8_t m, int sz) {
        if (found >= 0) return;
        if (m == MARK_ID) { want = data[p + 1] == track && data[p + 3] == sector; return; }
        if ((m == MARK_DATA || m == MARK_DELETED) && want && (size_t)p + 1 + sz + 2 <= len) {
            found = p;
            deleted = (m == MARK_DELETED);
            size = sz;
        }
        want = false;
    });
    return found;
}

void ibm_fm_put_sector(uint8_t * data, size_t len, int mark, const uint8_t * src, int size, bool deleted)
{
    if (mark < 0 || (size_t)mark + 1 + size + 2 > len) return;
    data[mark] = deleted ? MARK_DELETED : MARK_DATA;
    memcpy(data + mark + 1, src, size);
    const uint16_t crc = ibm_crc(0xFFFF, data + mark, 1 + (size_t)size);
    data[mark + 1 + size] = (uint8_t)(crc >> 8);
    data[mark + 2 + size] = (uint8_t)(crc & 0xFF);
}

//----------------------------- DVK MX -------------------------------------//

void dvk_mx_format_track(BYTES & data, int track, int side, const uint8_t * flat)
{
    data.assign(DVK_MX_TRACK_WORDS * 2, 0);
    size_t p = 8;     // zero words from the index to the sync word
    auto put = [&](uint16_t w) {
        data[p * 2] = (uint8_t)(w & 0xFF);
        data[p * 2 + 1] = (uint8_t)(w >> 8);
        p++;
    };
    put(DVK_MX_SYNC_WORD);
    put((uint16_t)track);
    for (int s = 0; s < DVK_MX_SECTORS; s++) {
        uint16_t sum = 0;
        for (int i = 0; i < DVK_MX_SECTOR_SIZE; i += 2) {
            const uint16_t w = (flat != nullptr)
                ? (uint16_t)(flat[s * DVK_MX_SECTOR_SIZE + i] | (flat[s * DVK_MX_SECTOR_SIZE + i + 1] << 8)) : 0;
            put(w);
            sum = (uint16_t)(sum + w);
        }
        put(sum);
    }
    for (int i = 0; i < 3; i++) put((uint16_t)(0101400 | (track * 2 + side)));
}

bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out)
{
    const size_t words = len / 2;
    size_t p = 0;
    while (p < words && (data[p * 2] | (data[p * 2 + 1] << 8)) != DVK_MX_SYNC_WORD) p++;
    p += 2;     // the sync word and the track number
    if (p + DVK_MX_SECTORS * (DVK_MX_SECTOR_SIZE / 2 + 1) > words) return false;
    for (int s = 0; s < DVK_MX_SECTORS; s++) {
        memcpy(out + s * DVK_MX_SECTOR_SIZE, data + p * 2, DVK_MX_SECTOR_SIZE);
        p += DVK_MX_SECTOR_SIZE / 2 + 1;   // the words of the sector and its sum
    }
    return true;
}

//----------------------------- RT-11 disks as cells ------------------------//

int rt11_track_kind(const std::string & type_id)
{
    if (type_id == "TYPE_RT11:DX") return 2;
    if (type_id.rfind("TYPE_RT11:MX", 0) == 0) return 3;
    if (type_id.rfind("TYPE_RT11:MY", 0) == 0 || type_id.rfind("TYPE_RT11:MZ", 0) == 0) return 1;
    return 0;
}

void rt11_hfe_params(int kind, HfeImage & img)
{
    img.interface_mode = GENERIC_SHUGGART_DD_FLOPPYMODE;
    switch (kind) {
    case 2:  img.bitrate = 250; img.rpm = 360; img.encoding = ISOIBM_FM_ENCODING; break;
    // 32 cells of 4 us a word: data at 125 kbit/s
    case 3:  img.bitrate = 125; img.rpm = 300; img.encoding = ISOIBM_FM_ENCODING; break;
    default: img.bitrate = 250; img.rpm = 300; img.encoding = ISOIBM_MFM_ENCODING; break;
    }
}

void rt11_track_cells(int kind, int track, int side, int sectors, int sector_size, const uint8_t * flat, BYTES & cells)
{
    BYTES data, special;
    if (kind == 3) {
        // A word goes highest bit first; the turn is 1562.5 words, the last
        // half word is zeros
        dvk_mx_format_track(data, track, side, flat);
        for (size_t k = 0; k + 1 < data.size(); k += 2) std::swap(data[k], data[k + 1]);
        data.push_back(0);
        fm_encode(data.data(), nullptr, data.size(), cells);
    } else if (kind == 2) {
        ibm_fm_format_track(data, special, track, side, sectors, sector_size, flat, 0);
        fm_encode(data.data(), special.data(), data.size(), cells);
    } else {
        ibm_mfm_format_track(data, special, track, side, sectors, sector_size, flat, 0);
        mfm_encode(data.data(), special.data(), data.size(), cells);
    }
}

bool rt11_track_sectors(int kind, const BYTES & cells, int sectors, int sector_size, uint8_t * out, uint32_t & missing)
{
    const size_t len = cells.size() / 2;        // 16 cells a byte
    BYTES data, special;
    missing = (sectors >= 32) ? 0xFFFFFFFFu : ((1u << sectors) - 1);
    if (kind == 3) {
        fm_decode(cells, len, data, special);
        for (size_t k = 0; k + 1 < data.size(); k += 2) std::swap(data[k], data[k + 1]);
        if (!dvk_mx_read_track(data.data(), data.size(), out)) return false;
        missing = 0;
        return true;
    }
    uint16_t deleted = 0;
    int found = 0;
    // Which sectors came: a sector of zeros is a sector too, so their places
    // are marked by filling the output with a pattern first
    std::vector<uint8_t> seen((size_t)sectors * sector_size, 0xE5);
    if (kind == 2) {
        fm_decode(cells, len, data, special);
        ibm_fm_read_track(data.data(), data.size(), sectors, sector_size, seen.data(), deleted, found);
    } else {
        mfm_decode(cells, len, data, special);
        ibm_mfm_read_track(data.data(), data.size(), sectors, sector_size, seen.data(), deleted, found);
    }
    if (found == 0) return false;
    // A second pass with another filler tells a found sector of E5 bytes
    // from one that is not there
    std::vector<uint8_t> seen2((size_t)sectors * sector_size, 0x00);
    if (kind == 2) ibm_fm_read_track(data.data(), data.size(), sectors, sector_size, seen2.data(), deleted, found);
    else           ibm_mfm_read_track(data.data(), data.size(), sectors, sector_size, seen2.data(), deleted, found);
    for (int s = 0; s < sectors; s++) {
        const size_t o = (size_t)s * sector_size;
        if (memcmp(seen.data() + o, seen2.data() + o, sector_size) == 0) {
            memcpy(out + o, seen.data() + o, sector_size);
            if (s < 32) missing &= ~(1u << s);
        }
    }
    return true;
}

int rt11_hfe_kind(const HfeImage & img)
{
    if (img.cells.empty() || img.cells[0].empty()) return 0;
    const BYTES & cells = img.cells[0];
    const size_t len = cells.size() / 2;
    BYTES data, special;
    uint16_t deleted = 0;
    int found = 0;
    std::vector<uint8_t> tmp(DVK_MX_SECTORS * DVK_MX_SECTOR_SIZE > 26 * 128 ? DVK_MX_SECTORS * DVK_MX_SECTOR_SIZE : 26 * 128);

    mfm_decode(cells, len, data, special);
    std::vector<uint8_t> mfm(10 * 512);
    if (ibm_mfm_read_track(data.data(), data.size(), 10, 512, mfm.data(), deleted, found) && found >= 5) return 1;

    fm_decode(cells, len, data, special);
    if (ibm_fm_read_track(data.data(), data.size(), 26, 128, tmp.data(), deleted, found) && found >= 13) return 2;
    for (size_t k = 0; k + 1 < data.size(); k += 2) std::swap(data[k], data[k + 1]);
    if (dvk_mx_read_track(data.data(), data.size(), tmp.data())) return 3;
    return 0;
}

} // namespace
