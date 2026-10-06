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

    const uint8_t MFM_GAP_BYTE    = 0x4E;
    const uint8_t MFM_SYNC_BYTE   = 0xA1;
    const uint8_t MFM_INDEX_SYNC  = 0xC2;
    const int     MFM_SYNC_ZEROS  = 12;
    const uint8_t FM_GAP_BYTE     = 0xFF;
    const int     FM_SYNC_ZEROS   = 6;

    const uint8_t MARK_INDEX      = 0xFC;
    const uint8_t MARK_ID         = 0xFE;
    const uint8_t MARK_DATA       = 0xFB;
    const uint8_t MARK_DELETED    = 0xF8;

    // How far after an ID field its data mark may be: gap 2 and the sync
    // bytes, with room for the wider gaps some controllers write
    const size_t  MFM_DATA_WINDOW = 80;
    const size_t  FM_DATA_WINDOW  = 60;

    // The gap between sectors as the IBM format tables give it
    int default_gap3(bool mfm, int size_code)
    {
        static const int mfm_gap3[] = {32, 54, 84, 116};
        static const int fm_gap3[]  = {27, 42, 58, 58};
        const int i = (size_code < 3) ? size_code : 3;
        return mfm ? mfm_gap3[i] : fm_gap3[i];
    }

    uint32_t all_sectors(int sectors)
    {
        return (sectors >= 32) ? 0xFFFFFFFFu : ((1u << sectors) - 1);
    }

    // Sectors into their places by the numbers in their ID fields. A copy whose
    // data passes the CRC wins over one that does not; the number placed
    int place_sectors(const IbmTrackFields & fields, const uint8_t * data, int first, int sectors, int sector_size,
                      uint8_t * out, TrackStatus & status)
    {
        status.missing = all_sectors(sectors);
        status.bad_crc = 0;
        status.deleted = 0;
        uint32_t good = 0;
        int found = 0;
        for (const IbmSectorField & f : fields.sectors) {
            if (!f.id_crc_ok || f.data_pos == IBM_NO_DATA || f.size() != sector_size) continue;
            const int n = f.sector - first;
            if (n < 0 || n >= sectors || n >= 32) continue;
            const uint32_t bit = 1u << n;
            if (good & bit) continue;
            if (!(status.missing & bit) && !f.data_crc_ok) continue;
            memcpy(out + (size_t)n * sector_size, data + f.data_pos + 1, sector_size);
            if (status.missing & bit) found++;
            status.missing &= ~bit;
            if (f.data_crc_ok) {
                good |= bit;
                status.bad_crc &= ~bit;
            } else
                status.bad_crc |= bit;
            if (f.deleted) status.deleted |= bit;
            else           status.deleted &= ~bit;
        }
        return found;
    }

    bool read_track(const uint8_t * data, size_t len, bool mfm, int sectors, int sector_size, uint8_t * out,
                    uint16_t & deleted, int & found)
    {
        IbmTrackFields fields;
        ibm_track_fields(data, len, nullptr, mfm, fields);
        TrackStatus status;
        found = place_sectors(fields, data, 1, sectors, sector_size, out, status);
        deleted = (uint16_t)(status.deleted & 0xFFFF);
        return found > 0;
    }

}

//----------------------------- IBM fields ---------------------------------//

int ibm_size_code(int sector_size)
{
    int code = 0;
    while ((128 << code) < sector_size && code < 6) code++;
    return code;
}

void ibm_track_fields(const uint8_t * data, size_t len, const uint8_t * special, bool mfm, IbmTrackFields & out)
{
    out.index_marks.clear();
    out.sectors.clear();

    // An MFM CRC runs from the first of the three A1 before the mark
    static const uint8_t sync[3] = {MFM_SYNC_BYTE, MFM_SYNC_BYTE, MFM_SYNC_BYTE};
    const uint16_t crc_preset = mfm ? ibm_crc(0xFFFF, sync, 3) : 0xFFFF;

    auto mark_at = [&](size_t p) -> uint8_t {
        const uint8_t m = data[p];
        if (m != MARK_INDEX && m != MARK_ID && m != MARK_DATA && m != MARK_DELETED) return 0;
        if (mfm) {
            const uint8_t s = (m == MARK_INDEX) ? MFM_INDEX_SYNC : MFM_SYNC_BYTE;
            if (p < 3 || data[p - 1] != s || data[p - 2] != s || data[p - 3] != s) return 0;
            // The decoder flags A1 only: C2 is not a sync for it
            if (special != nullptr && m != MARK_INDEX && !special[p - 1]) return 0;
        } else {
            if (special != nullptr) {
                if (!special[p]) return 0;
            } else
            if (p == 0 || data[p - 1] != 0) return 0;
        }
        return m;
    };
    // n bytes after the mark at p, then the CRC high byte first
    auto crc_ok = [&](size_t p, size_t n) -> bool {
        const uint16_t crc = ibm_crc(crc_preset, data + p, n + 1);
        return data[p + 1 + n] == (uint8_t)(crc >> 8) && data[p + 2 + n] == (uint8_t)(crc & 0xFF);
    };
    const size_t window = mfm ? MFM_DATA_WINDOW : FM_DATA_WINDOW;

    size_t p = 0;
    while (p < len) {
        const uint8_t m = mark_at(p);
        if (m == MARK_INDEX) {
            out.index_marks.push_back(p);
            p++;
            continue;
        }
        if (m != MARK_ID) {
            // Nothing, or a data field with no ID in front of it
            p++;
            continue;
        }
        if (p + 7 > len) break;
        IbmSectorField f;
        f.id_pos = p;
        f.data_pos = IBM_NO_DATA;
        f.track = data[p + 1];
        f.side = data[p + 2];
        f.sector = data[p + 3];
        f.size_code = data[p + 4];
        f.id_crc_ok = crc_ok(p, 4);
        f.data_crc_ok = false;
        f.deleted = false;
        p += 7;
        // A controller takes the data only after an ID it could read
        if (f.id_crc_ok) {
            const size_t size = (size_t)f.size();
            for (size_t q = p; q < len && q < p + window; q++) {
                const uint8_t dm = mark_at(q);
                if (dm == MARK_ID || dm == MARK_INDEX) break;
                if (dm != MARK_DATA && dm != MARK_DELETED) continue;
                if (q + 1 + size + 2 <= len) {
                    f.data_pos = q;
                    f.deleted = (dm == MARK_DELETED);
                    f.data_crc_ok = crc_ok(q, size);
                    p = q + 1 + size + 2;
                }
                break;
            }
        }
        out.sectors.push_back(f);
    }
}

bool ibm_format_track(const IbmTrackFormat & fmt, int track, int side, const uint8_t * flat, uint32_t deleted,
                      BYTES & data, BYTES & special)
{
    const bool mfm = fmt.mfm;
    const int len = fmt.track_bytes;
    const int zeros = mfm ? MFM_SYNC_ZEROS : FM_SYNC_ZEROS;
    const int mark_len = mfm ? 4 : 1;                                 // the sync bytes and the mark
    const int code = ibm_size_code(fmt.sector_size);
    const TrackGaps & g = fmt.gaps;
    const int gap4a = g.index_mark ? ((g.gap4a >= 0) ? g.gap4a : (mfm ? 80 : 40)) : 0;
    const int gap1 = (g.gap1 >= 0) ? g.gap1 : (mfm ? 50 : 26);
    const int gap2 = (g.gap2 >= 0) ? g.gap2 : (mfm ? 22 : 11);
    const int head = gap4a + (g.index_mark ? zeros + mark_len : 0) + gap1;
    const int sector_len = zeros + mark_len + 4 + 2 + gap2 + zeros + mark_len + fmt.sector_size + 2;
    int gap3 = g.gap3;
    if (gap3 < 0) {
        gap3 = default_gap3(mfm, code);
        const int room = (fmt.sectors > 0) ? (len - head - fmt.sectors * sector_len) / fmt.sectors : gap3;
        if (room < gap3) gap3 = (room > 0) ? room : 0;
    }

    data.assign(len, mfm ? MFM_GAP_BYTE : FM_GAP_BYTE);
    special.assign(len, 0);
    bool fits = true;
    int p = 0;
    auto put = [&](uint8_t b, bool mark) {
        if (p >= len) { fits = false; return; }
        special[p] = mark ? 1 : 0;
        data[p++] = b;
    };
    auto skip = [&](int n) { p += n; if (p > len) fits = false; };
    auto put_zeros = [&]() { for (int i = 0; i < zeros; i++) put(0, false); };
    // The mark with the sync bytes in front of it (MFM) or with a clock of its own (FM)
    auto put_mark = [&](uint8_t m) {
        if (mfm) {
            const uint8_t s = (m == MARK_INDEX) ? MFM_INDEX_SYNC : MFM_SYNC_BYTE;
            for (int i = 0; i < 3; i++) put(s, true);
            put(m, false);
        } else
            put(m, true);
    };
    auto put_crc = [&](int start) {
        if (!fits) return;
        const uint16_t crc = ibm_crc(0xFFFF, &data[start], p - start);
        put((uint8_t)(crc >> 8), false);
        put((uint8_t)(crc & 0xFF), false);
    };

    skip(gap4a);
    if (g.index_mark) {
        put_zeros();
        put_mark(MARK_INDEX);
    }
    skip(gap1);
    for (int sect = 0; sect < fmt.sectors && fits; sect++) {
        put_zeros();
        int start = p;
        put_mark(MARK_ID);
        put((uint8_t)track, false);
        put((uint8_t)side, false);
        put((uint8_t)(fmt.first_sector + sect), false);
        put((uint8_t)code, false);
        put_crc(start);

        skip(gap2);
        put_zeros();
        start = p;
        put_mark((sect < 32 && (deleted & (1u << sect))) ? MARK_DELETED : MARK_DATA);
        for (int i = 0; i < fmt.sector_size; i++)
            put((flat != nullptr) ? flat[(size_t)sect * fmt.sector_size + i] : 0, false);
        put_crc(start);
        if (sect + 1 < fmt.sectors) skip(gap3);
    }
    return fits;
}

//----------------------------- IBM MFM ------------------------------------//

void ibm_mfm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                          const uint8_t * flat, uint16_t deleted)
{
    IbmTrackFormat fmt;
    fmt.sectors = sectors;
    fmt.sector_size = sector_size;
    fmt.gaps.index_mark = false;
    fmt.gaps.gap1 = 42;
    fmt.gaps.gap2 = 22;
    fmt.gaps.gap3 = 36;
    ibm_format_track(fmt, track, side, flat, deleted, data, special);
}

bool ibm_mfm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                        uint16_t & deleted, int & found)
{
    return read_track(data, len, true, sectors, sector_size, out, deleted, found);
}

// Sync marks by the bytes alone: three A1 after zeros and before FE, FB or F8
void ibm_mfm_find_marks(const uint8_t * data, size_t len, BYTES & special)
{
    special.assign(len, 0);
    for (size_t p = 1; p + 3 < len; p++)
        if (data[p - 1] == 0 && data[p] == MFM_SYNC_BYTE && data[p + 1] == MFM_SYNC_BYTE && data[p + 2] == MFM_SYNC_BYTE
            && (data[p + 3] == MARK_ID || data[p + 3] == MARK_DATA || data[p + 3] == MARK_DELETED))
            special[p] = special[p + 1] = special[p + 2] = 1;
}

//----------------------------- IBM 3740 FM --------------------------------//

void ibm_fm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                         const uint8_t * flat, uint16_t deleted)
{
    IbmTrackFormat fmt;
    fmt.mfm = false;
    fmt.track_bytes = IBM_FM_TRACK_BYTES;
    fmt.sectors = sectors;
    fmt.sector_size = sector_size;
    fmt.gaps.gap4a = 40;
    fmt.gaps.gap1 = 26;
    fmt.gaps.gap2 = 11;
    fmt.gaps.gap3 = 27;
    ibm_format_track(fmt, track, side, flat, deleted, data, special);
}

bool ibm_fm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                       uint16_t & deleted, int & found)
{
    return read_track(data, len, false, sectors, sector_size, out, deleted, found);
}

void ibm_fm_find_marks(const uint8_t * data, size_t len, BYTES & special)
{
    special.assign(len, 0);
    IbmTrackFields fields;
    ibm_track_fields(data, len, nullptr, false, fields);
    for (size_t p : fields.index_marks) special[p] = 1;
    for (const IbmSectorField & f : fields.sectors) {
        special[f.id_pos] = 1;
        if (f.data_pos != IBM_NO_DATA) special[f.data_pos] = 1;
    }
}

int ibm_fm_find_sector(const uint8_t * data, size_t len, int track, int sector, bool & deleted, int & size)
{
    IbmTrackFields fields;
    ibm_track_fields(data, len, nullptr, false, fields);
    for (const IbmSectorField & f : fields.sectors)
        if (f.track == track && f.sector == sector && f.data_pos != IBM_NO_DATA) {
            deleted = f.deleted;
            size = f.size();
            return (int)f.data_pos;
        }
    return -1;
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

bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out, uint32_t & bad)
{
    auto word = [&](size_t i) { return (uint16_t)(data[i * 2] | (data[i * 2 + 1] << 8)); };
    const size_t words = len / 2;
    bad = 0;
    size_t p = 0;
    while (p < words && word(p) != DVK_MX_SYNC_WORD) p++;
    p += 2;     // the sync word and the track number
    if (p + DVK_MX_SECTORS * (DVK_MX_SECTOR_SIZE / 2 + 1) > words) return false;
    for (int s = 0; s < DVK_MX_SECTORS; s++) {
        uint16_t sum = 0;
        for (int i = 0; i < DVK_MX_SECTOR_SIZE / 2; i++) sum = (uint16_t)(sum + word(p + i));
        memcpy(out + s * DVK_MX_SECTOR_SIZE, data + p * 2, DVK_MX_SECTOR_SIZE);
        p += DVK_MX_SECTOR_SIZE / 2;
        if (word(p) != sum) bad |= 1u << s;
        p++;
    }
    return true;
}

bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out)
{
    uint32_t bad = 0;
    return dvk_mx_read_track(data, len, out, bad);
}

//----------------------------- Disks as cells -----------------------------//

TrackLayout track_layout_by_name(const std::string & name)
{
    if (name == "ibm-mfm") return TrackLayout::IbmMfm;
    if (name == "ibm-fm")  return TrackLayout::IbmFm;
    if (name == "dvk-mx")  return TrackLayout::DvkMx;
    return TrackLayout::None;
}

std::string track_layout_name(TrackLayout layout)
{
    switch (layout) {
    case TrackLayout::IbmMfm: return "ibm-mfm";
    case TrackLayout::IbmFm:  return "ibm-fm";
    case TrackLayout::DvkMx:  return "dvk-mx";
    default:                  return "";
    }
}

namespace {

    unsigned layout_bitrate(const DiskFormatParams & format)
    {
        if (format.bitrate != 0) return format.bitrate;
        return (format.layout == TrackLayout::DvkMx) ? 125 : 250;
    }

    unsigned layout_rpm(const DiskFormatParams & format)
    {
        if (format.rpm != 0) return format.rpm;
        return (format.layout == TrackLayout::IbmFm) ? 360 : 300;
    }

    IbmTrackFormat ibm_format(const DiskFormatParams & format)
    {
        IbmTrackFormat fmt;
        fmt.mfm = (format.layout == TrackLayout::IbmMfm);
        fmt.track_bytes = track_bytes(format);
        fmt.sectors = (int)format.sectors;
        fmt.sector_size = (int)format.sector_size;
        fmt.first_sector = (int)format.sector_base;
        fmt.gaps = format.gaps;
        return fmt;
    }

}

void track_layout_defaults(DiskFormatParams & format)
{
    if (format.layout == TrackLayout::None) return;
    format.bitrate = layout_bitrate(format);
    format.rpm = layout_rpm(format);
    format.track_encoding = (format.layout == TrackLayout::IbmMfm) ? ISOIBM_MFM_ENCODING : ISOIBM_FM_ENCODING;
}

int track_bytes(const DiskFormatParams & format)
{
    // kbit/s * 1000 / 8 bits a byte * 60 s / rpm
    return (int)(layout_bitrate(format) * 7500 / layout_rpm(format));
}

void track_hfe_params(const DiskFormatParams & format, HfeImage & img)
{
    img.tracks = (int)format.tracks;
    img.sides = (int)format.heads;
    img.bitrate = (uint16_t)layout_bitrate(format);
    img.rpm = (uint16_t)layout_rpm(format);
    img.encoding = (format.layout == TrackLayout::IbmMfm) ? ISOIBM_MFM_ENCODING : ISOIBM_FM_ENCODING;
    img.interface_mode = (img.bitrate >= 500) ? IBMPC_HD_FLOPPYMODE : GENERIC_SHUGGART_DD_FLOPPYMODE;
    img.cells.assign((size_t)img.tracks * img.sides, BYTES());
}

bool track_to_cells(const DiskFormatParams & format, int track, int side, const uint8_t * flat, BYTES & cells)
{
    BYTES data, special;
    switch (format.layout) {
    case TrackLayout::DvkMx:
        // A word goes highest bit first; the turn is 1562.5 words, the last
        // half word is zeros
        dvk_mx_format_track(data, track, side, flat);
        for (size_t k = 0; k + 1 < data.size(); k += 2) std::swap(data[k], data[k + 1]);
        data.resize((size_t)track_bytes(format), 0);
        fm_encode(data.data(), nullptr, data.size(), cells);
        return true;
    case TrackLayout::IbmFm:
    case TrackLayout::IbmMfm: {
        const IbmTrackFormat fmt = ibm_format(format);
        const bool fits = ibm_format_track(fmt, track, side, flat, 0, data, special);
        if (fmt.mfm) mfm_encode(data.data(), special.data(), data.size(), cells);
        else         fm_encode(data.data(), special.data(), data.size(), cells);
        return fits;
    }
    default:
        cells.clear();
        return false;
    }
}

void track_decode(const DiskFormatParams & format, const BYTES & cells, BYTES & data, IbmTrackFields & fields)
{
    const size_t len = cells.size() / 2;        // 16 cells a byte
    BYTES special;
    fields.index_marks.clear();
    fields.sectors.clear();
    if (format.layout == TrackLayout::IbmMfm) {
        mfm_decode(cells, len, data, special);
        ibm_track_fields(data.data(), data.size(), special.data(), true, fields);
    } else {
        fm_decode(cells, len, data, special);
        if (format.layout == TrackLayout::IbmFm)
            ibm_track_fields(data.data(), data.size(), special.data(), false, fields);
        else
        if (format.layout == TrackLayout::DvkMx)
            for (size_t k = 0; k + 1 < data.size(); k += 2) std::swap(data[k], data[k + 1]);
    }
}

bool track_from_cells(const DiskFormatParams & format, const BYTES & cells, uint8_t * out, TrackStatus & status)
{
    const int sectors = (int)format.sectors;
    status = TrackStatus();
    status.missing = all_sectors(sectors);
    if (format.layout == TrackLayout::None) return false;

    BYTES data;
    IbmTrackFields fields;
    track_decode(format, cells, data, fields);
    if (format.layout == TrackLayout::DvkMx) {
        uint32_t bad = 0;
        if (!dvk_mx_read_track(data.data(), data.size(), out, bad)) return false;
        status.missing = 0;
        status.bad_crc = bad;
        return true;
    }
    return place_sectors(fields, data.data(), (int)format.sector_base, sectors, (int)format.sector_size, out, status) > 0;
}

void hfe_to_flat(const HfeImage & img, const DiskFormatParams & format, BYTES & out, BadSectorTable * bad)
{
    const size_t track_size = (size_t)format.sectors * format.sector_size;
    out.assign(track_size * format.heads * format.tracks, 0);
    for (unsigned t = 0; t < format.tracks; t++)
        for (unsigned h = 0; h < format.heads; h++) {
            TrackStatus status;
            status.missing = all_sectors((int)format.sectors);
            if ((int)t < img.tracks && (int)h < img.sides)
                track_from_cells(format, img.cells[(size_t)t * img.sides + h], out.data() + (t * format.heads + h) * track_size, status);
            if (bad == nullptr) continue;
            const uint32_t failed = status.missing | status.bad_crc;
            for (unsigned s = 0; s < format.sectors && s < 32; s++)
                if (failed & (1u << s)) bad->insert(bad_sector_key(h, t, s + format.sector_base));
        }
}

namespace {

    // What the IBM fields of a few tracks say about the geometry
    bool probe_ibm(const HfeImage & img, bool mfm, DiskFormatParams & format)
    {
        DiskFormatParams probe;
        probe.layout = mfm ? TrackLayout::IbmMfm : TrackLayout::IbmFm;
        int min_id = 256, max_id = -1, good = 0;
        int codes[8] = {0};
        for (int t = 0; t < img.tracks && t < 3; t++) {
            BYTES data;
            IbmTrackFields fields;
            track_decode(probe, img.cells[(size_t)t * img.sides], data, fields);
            for (const IbmSectorField & f : fields.sectors) {
                if (!f.id_crc_ok || f.data_pos == IBM_NO_DATA || f.size_code > 7) continue;
                good++;
                codes[f.size_code]++;
                if (f.sector < min_id) min_id = f.sector;
                if (f.sector > max_id) max_id = f.sector;
            }
        }
        // Two readable sectors are more than chance gives on a track of
        // another kind: an ID needs its marks and its CRC to agree
        if (good < 2) return false;
        int code = 0;
        for (int i = 1; i < 8; i++) if (codes[i] > codes[code]) code = i;
        format.layout = probe.layout;
        format.sector_base = (unsigned)min_id;
        format.sectors = (unsigned)(max_id - min_id + 1);
        format.sector_size = 128u << code;
        return true;
    }

    bool probe_mx(const HfeImage & img)
    {
        DiskFormatParams probe;
        probe.layout = TrackLayout::DvkMx;
        BYTES data;
        IbmTrackFields fields;
        track_decode(probe, img.cells[0], data, fields);
        BYTES out(DVK_MX_SECTORS * DVK_MX_SECTOR_SIZE);
        uint32_t bad = 0;
        return dvk_mx_read_track(data.data(), data.size(), out.data(), bad) && bad != 0x7FF;
    }

}

bool hfe_probe_format(const HfeImage & img, DiskFormatParams & format)
{
    if (img.tracks == 0 || img.sides == 0 || img.cells.empty() || img.cells[0].empty()) return false;
    if (!probe_ibm(img, true, format) && !probe_ibm(img, false, format)) {
        if (!probe_mx(img)) return false;
        format.layout = TrackLayout::DvkMx;
        format.sector_base = 1;
        format.sectors = DVK_MX_SECTORS;
        format.sector_size = DVK_MX_SECTOR_SIZE;
    }
    format.heads = (unsigned)img.sides;
    format.tracks = (unsigned)img.tracks;
    format.bitrate = img.bitrate;
    format.rpm = img.rpm;
    format.expected_size = format.heads * format.tracks * format.sectors * format.sector_size;
    return true;
}

} // namespace
