// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: HFE container (HxC Floppy Emulator v1) and FM/MFM bit cell codecs

#include <cstring>

#include "dsk_tools/core.h"

namespace dsk_tools {

namespace {

    inline bool cell(const BYTES & cells, size_t bit)
    {
        return (cells[bit >> 3] >> (bit & 7)) & 1;
    }

    inline void put_cell(BYTES & cells, size_t bit, bool v)
    {
        if (v) cells[bit >> 3] |= (uint8_t)(1u << (bit & 7));
        else   cells[bit >> 3] &= (uint8_t)~(1u << (bit & 7));
    }

    // 16 cells from a bit position, the first one is the highest bit
    inline uint16_t window(const BYTES & cells, size_t bit)
    {
        uint16_t w = 0;
        for (int i = 0; i < 16; i++) w = (uint16_t)((w << 1) | (cell(cells, bit + i) ? 1 : 0));
        return w;
    }

    inline uint8_t data_bits(uint16_t w)
    {
        uint8_t d = 0;
        for (int i = 0; i < 8; i++) d = (uint8_t)((d << 1) | ((w >> (14 - 2 * i)) & 1));
        return d;
    }

    inline uint8_t clock_bits(uint16_t w)
    {
        uint8_t c = 0;
        for (int i = 0; i < 8; i++) c = (uint8_t)((c << 1) | ((w >> (15 - 2 * i)) & 1));
        return c;
    }

    // Bytes out of the cells, the byte grid taken anew at every mark: a
    // track written by a drive need not start on a byte boundary, nor keep
    // the same phase from one field to the next. The bytes before the first
    // mark follow its phase, and so does the end of the track after the last
    void decode_segments(const BYTES & cells, size_t bits, const std::vector<size_t> & marks,
                         size_t len, BYTES & data, BYTES & special)
    {
        data.assign(len, 0);
        special.assign(len, 0);
        if (bits < 16 || len == 0) return;
        const size_t first_phase = marks.empty() ? 0 : marks[0] % 16;
        // Segment k starts at the byte of mark k and runs up to the next one
        for (size_t k = 0; k <= marks.size(); k++) {
            const size_t phase = (k == 0) ? first_phase : marks[k - 1] % 16;
            const size_t from_byte = (k == 0) ? 0 : marks[k - 1] / 16;
            const size_t to_byte = (k < marks.size()) ? marks[k] / 16 : len;
            for (size_t i = from_byte; i < to_byte && i < len; i++) {
                const size_t bit = i * 16 + phase;
                if (bit + 16 > bits) break;
                data[i] = data_bits(window(cells, bit));
            }
            if (k < marks.size() && marks[k] / 16 < len) special[marks[k] / 16] = 1;
        }
    }

}

//----------------------------- Container ----------------------------------//

Result hfe_read(const BYTES & file, HfeImage & img)
{
    if (file.size() < 512 + sizeof(HXC_HFE_TRACK))
        return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "File is too small"));
    HXC_HFE_HEADER hdr;
    memcpy(&hdr, file.data(), sizeof(hdr));
    if (memcmp(hdr.HEADERSIGNATURE, "HXCPICFE", 8) != 0)
        return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Invalid HFE signature"));
    if (hdr.number_of_track == 0 || hdr.number_of_side == 0 || hdr.number_of_side > 2)
        return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Invalid HFE parameters"));

    img.tracks = hdr.number_of_track;
    img.sides = hdr.number_of_side;
    img.bitrate = hdr.bitRate;
    img.rpm = hdr.floppyRPM;
    img.encoding = hdr.track_encoding;
    img.interface_mode = hdr.floppyinterfacemode;
    img.cells.assign((size_t)img.tracks * img.sides, BYTES());

    const size_t lut = (size_t)hdr.track_list_offset * 512;
    if (lut + (size_t)img.tracks * sizeof(HXC_HFE_TRACK) > file.size())
        return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Invalid HFE parameters"));

    for (int t = 0; t < img.tracks; t++) {
        HXC_HFE_TRACK ti;
        memcpy(&ti, file.data() + lut + (size_t)t * sizeof(HXC_HFE_TRACK), sizeof(ti));
        const size_t base = (size_t)ti.offset * 512;
        // track_len is both sides together: every 512 byte block holds 256
        // bytes of side 0, then 256 of side 1
        const size_t side_len = ti.track_len / 2;
        const size_t blocks = (ti.track_len + 511) / 512;
        if (base + blocks * 512 > file.size())
            return Result::error(ErrorCode::LoadIncorrectFile, QT_TRANSLATE_NOOP("errors", "Invalid HFE parameters"));
        for (int s = 0; s < img.sides; s++) {
            BYTES &out = img.cells[(size_t)t * img.sides + s];
            out.reserve(side_len);
            for (size_t b = 0; b < blocks && out.size() < side_len; b++) {
                const uint8_t * part = file.data() + base + b * 512 + (size_t)s * 256;
                const size_t n = (side_len - out.size() < 256) ? side_len - out.size() : 256;
                out.insert(out.end(), part, part + n);
            }
        }
    }
    return Result::ok();
}

void hfe_write(const HfeImage & img, BYTES & file)
{
    file.clear();
    HXC_HFE_HEADER hdr;
    memset(&hdr, 0xFF, sizeof(hdr));
    memcpy(hdr.HEADERSIGNATURE, "HXCPICFE", 8);
    hdr.formatrevision = 0;
    hdr.number_of_track = (uint8_t)img.tracks;
    hdr.number_of_side = (uint8_t)img.sides;
    hdr.track_encoding = img.encoding;
    hdr.bitRate = img.bitrate;
    hdr.floppyRPM = img.rpm;
    hdr.floppyinterfacemode = img.interface_mode;
    hdr.write_protected = 0xFF;
    hdr.track_list_offset = 1;
    hdr.write_allowed = 0xFF;
    const uint8_t * h = reinterpret_cast<const uint8_t*>(&hdr);
    file.insert(file.end(), h, h + sizeof(hdr));
    file.resize(512, 0xFF);

    // The track list: one block is enough for 128 tracks
    const size_t lut_blocks = ((size_t)img.tracks * sizeof(HXC_HFE_TRACK) + 511) / 512;
    const size_t lut = file.size();
    file.resize(lut + lut_blocks * 512, 0xFF);

    for (int t = 0; t < img.tracks; t++) {
        size_t side_len = 0;
        for (int s = 0; s < img.sides; s++) {
            const BYTES &c = img.cells[(size_t)t * img.sides + s];
            if (c.size() > side_len) side_len = c.size();
        }
        const size_t blocks = (side_len + 255) / 256;
        HXC_HFE_TRACK ti;
        ti.offset = (uint16_t)(file.size() / 512);
        ti.track_len = (uint16_t)(side_len * 2);
        memcpy(file.data() + lut + (size_t)t * sizeof(ti), &ti, sizeof(ti));

        const size_t base = file.size();
        file.resize(base + blocks * 512, 0);
        for (int s = 0; s < 2; s++) {
            // A single sided image still has the second half of each block
            const BYTES &c = img.cells[(size_t)t * img.sides + (s < img.sides ? s : 0)];
            for (size_t b = 0; b < blocks; b++)
                for (size_t i = 0; i < 256; i++) {
                    const size_t k = b * 256 + i;
                    file[base + b * 512 + (size_t)s * 256 + i] = (s < img.sides && k < c.size()) ? c[k] : 0xAA;
                }
        }
    }
}

//----------------------------- MFM ----------------------------------------//

// A data cell is the bit; a clock cell is 1 between two zero bits. A sync
// byte drops one clock: A1 gives 4489, C2 gives 5224
void mfm_encode(const uint8_t * data, const uint8_t * special, size_t len, BYTES & cells)
{
    cells.assign(len * 2, 0);
    bool prev = false;
    size_t bit = 0;
    for (size_t i = 0; i < len; i++) {
        uint16_t w = 0;
        for (int b = 7; b >= 0; b--) {
            const bool d = (data[i] >> b) & 1;
            const bool c = !prev && !d;
            w = (uint16_t)((w << 2) | (c ? 2 : 0) | (d ? 1 : 0));
            prev = d;
        }
        if (special != nullptr && special[i]) {
            if (data[i] == 0xA1) w = 0x4489;
            else if (data[i] == 0xC2) w = 0x5224;
        }
        for (int k = 15; k >= 0; k--) put_cell(cells, bit++, (w >> k) & 1);
    }
}

// Only 4489 is a sync here: the 5224 of the index mark also turns up,
// shifted, inside an ordinary 4E gap (9254 9254 ...) and would move the byte
// grid there
void mfm_decode(const BYTES & cells, size_t len, BYTES & data, BYTES & special)
{
    const size_t bits = cells.size() * 8;
    std::vector<size_t> marks;
    for (size_t bit = 0; bit + 16 <= bits; bit++) {
        const uint16_t w = window(cells, bit);
        if (w == 0x4489) {
            marks.push_back(bit);
            bit += 15;
        }
    }
    decode_segments(cells, bits, marks, len, data, special);
}

//----------------------------- FM -----------------------------------------//

// Every data bit follows a clock cell of 1; an address mark has a clock
// pattern of its own: C7 for FE, FB, F8, D7 for the index mark FC
void fm_encode(const uint8_t * data, const uint8_t * special, size_t len, BYTES & cells)
{
    cells.assign(len * 2, 0);
    size_t bit = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t clock = 0xFF;
        if (special != nullptr && special[i]) clock = (data[i] == 0xFC) ? 0xD7 : 0xC7;
        for (int b = 7; b >= 0; b--) {
            put_cell(cells, bit++, (clock >> b) & 1);
            put_cell(cells, bit++, (data[i] >> b) & 1);
        }
    }
}

void fm_decode(const BYTES & cells, size_t len, BYTES & data, BYTES & special)
{
    const size_t bits = cells.size() * 8;
    std::vector<size_t> marks;
    for (size_t bit = 0; bit + 16 <= bits; bit++) {
        const uint16_t w = window(cells, bit);
        const uint8_t c = clock_bits(w), d = data_bits(w);
        if ((c == 0xC7 && (d == 0xFE || d == 0xFB || d == 0xF8)) || (c == 0xD7 && d == 0xFC)) {
            marks.push_back(bit);
            bit += 15;
        }
    }
    decode_segments(cells, bits, marks, len, data, special);
}

} // namespace
