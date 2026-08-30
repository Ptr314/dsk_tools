// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: AIM to HFE converter
//
// The algorithm follows agath-aim-to-hfe.pl by Oleksandr Kapitanenko
// <kapitan@portaone.com>, (c) 2014 PortaOne, Inc., with its default settings:
// index hole alignment is not corrected (fix_index_divided_sector_position = 0),
// the output is HFE v2, which allows a track to loop over the index hole.

#include "aim2hfe.h"

#include <cstring>
#include <string>
#include <vector>

#include "definitions.h"

namespace dsk_tools
{
    namespace {

        // AIM geometry: 160 tracks (80 cylinders * 2 sides),
        // each track is 6464 cells of (data byte, AIM command)
        const int AIM_TRACKS = 160;
        const int AIM_TRACK_CELLS = 6464;

        // An MFM track of 250 kbps at 300 RPM is exactly 6250 bytes
        const int MFM_TRACK_CELLS = 6250;

        // AIM commands
        const uint8_t AIM_DESYNC        = 0x01;
        const uint8_t AIM_TRACK_END     = 0x02;
        const uint8_t AIM_INDEX_START   = 0x03;
        const uint8_t AIM_INDEX_END     = 0x13;
        const uint8_t AIM_DESYNC_ALT    = 0x80;

        // Some dumps set both DESYNC flags at once
        const uint8_t AIM_DESYNC_BOTH   = AIM_DESYNC | AIM_DESYNC_ALT;

        // GAPs of this length or shorter are not deflated
        const int MINIMUM_GAP_LEN = 5;

        // Inflate GAPs shorter than MINIMUM_GAP_LEN up to it
        const bool INFLATE_SMALL_GAPS = true;

        // Sector data length expected after a DATA mark
        const int SECTOR_LEN = 256;

        // A GAP is filled with this byte when written to a disk
        const uint8_t GAP_FILLER = 0xAA;

        // Some AIM dumps report the tail of every long GAP as 0x00 instead of the
        // filler actually written to the disk. Such a cell is still a part of the
        // GAP, so it is counted and restored to GAP_FILLER
        const uint8_t GAP_UNREAD = 0x00;

        const int HFE_SIDES = 2;
        const int HFE_BLOCK = 512;
        const int HFE_BITRATE = 250;

        // A single AIM cell: a data byte and an AIM command. After the DESYNC
        // expansion the command field of the first cell of a GAP holds its length,
        // which can be longer than a byte, hence the type
        struct AIMCell
        {
            uint8_t data;
            int cmd;
        };

        typedef std::vector<AIMCell> AIMTrack;

        enum class DecoderState {PreGap, Desync, Marker, IndexMarker, DataMarker, SectorHeader, SectorData};
        enum class DeflateState {Payload, Gap, Skip};

        bool is_gap_filler(const uint8_t b)
        {
            return b == GAP_FILLER || b == GAP_UNREAD;
        }

        std::string track_msg(const int track, const std::string & message)
        {
            return "Track " + std::to_string(track) + ": " + message + "\n";
        }

        // A DESYNC sequence is stored in AIM as a payload 0xA4 (or 0xA4 0xFF)
        // followed by the DESYNC command itself, so the payload copy is removed
        void trim_desync_prefix(AIMTrack & track, int & gap_len)
        {
            if (track.size() >= 1 && track.back().data == 0xA4) {
                track.pop_back();
                gap_len -= 1;
            }
            if (track.size() >= 2 && track[track.size() - 2].data == 0xA4 && track.back().data == 0xFF) {
                track.resize(track.size() - 2);
                gap_len -= 2;
            }
        }

        // A GAP is complete, so its unread cells can be restored to the filler
        // byte a disk really holds there
        void restore_gap_filler(AIMTrack & track, const size_t gap_index)
        {
            for (size_t i = gap_index; i < track.size(); i++)
                if (track[i].data == GAP_UNREAD) track[i].data = GAP_FILLER;
        }

        // Expands every DESYNC command into the 0xA4 0xFF sequence written to a disk,
        // measures each GAP and stores its length into the first cell of the GAP
        AIMTrack expand_desync(const AIMTrack & src, const int track, int & gap_total_len, std::string & log, const bool verbose)
        {
            AIMTrack dst;
            dst.reserve(src.size());

            DecoderState state = DecoderState::PreGap;
            int gap_len = 0;
            size_t gap_index = 0;
            uint8_t exception_byte = 0xA4;
            int idam_cursor = 0;
            int data_cursor = 0;
            int index_count = 0;
            int index_hole_ptr = -1;
            int max_gap_len = 0;

            gap_total_len = 0;

            for (size_t i = 0; i < src.size(); i++) {
                const uint8_t aim_byte = src[i].data;
                uint8_t aim_command = src[i].cmd;

                if (aim_command == AIM_DESYNC_ALT || aim_command == AIM_DESYNC_BOTH) aim_command = AIM_DESYNC;
                if (aim_command == AIM_INDEX_START) {
                    aim_command = 0;
                    index_hole_ptr = static_cast<int>(i);
                }
                if (aim_command == AIM_INDEX_END) aim_command = 0;

                if (aim_command == AIM_TRACK_END) {
                    if (verbose) log += track_msg(track, "end of track command, skipping the rest of the AIM track");
                    trim_desync_prefix(dst, gap_len);
                    break;
                }

                if (aim_command > AIM_DESYNC) {
                    log += track_msg(track, "WARNING: unsupported AIM command " + std::to_string(aim_command) + ", ignoring");
                    aim_command = 0;
                }

                // Copy protection may write sector data over the disk index hole
                // or some payload at a GAP beginning
                if (state == DecoderState::PreGap && is_gap_filler(aim_byte)) {
                    gap_index = dst.size();
                    state = DecoderState::Desync;
                }
                if (aim_command == AIM_DESYNC) state = DecoderState::Desync;

                if (state == DecoderState::Desync) {
                    if (aim_command != AIM_DESYNC) {
                        if (!is_gap_filler(aim_byte) && aim_byte != exception_byte) {
                            state = DecoderState::PreGap;
                            gap_len = 0;
                            exception_byte = 0xA4;
                        } else {
                            // GAP
                            gap_len++;
                            if (aim_byte == 0xA4) exception_byte = 0xFF;
                            else
                            if (aim_byte == 0xFF) exception_byte = 0x00;
                        }
                        dst.push_back({aim_byte, aim_command});
                    } else {
                        // DESYNC
                        exception_byte = 0xA4;
                        trim_desync_prefix(dst, gap_len);

                        if (gap_len > 0) {
                            // End of a GAP
                            if (gap_index < dst.size()) restore_gap_filler(dst, gap_index);

                            if (INFLATE_SMALL_GAPS && gap_len < MINIMUM_GAP_LEN)
                                dst.insert(dst.end(), MINIMUM_GAP_LEN - gap_len, AIMCell{GAP_FILLER, 0});

                            if (gap_len > MINIMUM_GAP_LEN) {
                                if (gap_index < dst.size()) dst[gap_index].cmd = gap_len;
                                gap_total_len += gap_len;
                            }
                            if (gap_len > max_gap_len) max_gap_len = gap_len;
                            gap_len = -1;
                        } else {
                            log += track_msg(track, "WARNING: GAP expected, no GAP found");
                        }

                        // A DESYNC as it is written to a disk
                        dst.push_back({0xA4, AIM_DESYNC});
                        dst.push_back({0xFF, 0});
                        state = DecoderState::Marker;
                    }
                } else {
                    dst.push_back({aim_byte, aim_command});

                    if (state == DecoderState::Marker) {
                        if (aim_byte == 0x95) {
                            // Possible INDEX mark
                            state = DecoderState::IndexMarker;
                            idam_cursor = 0;
                        } else
                        if (aim_byte == 0x6A) {
                            // Possible DATA mark
                            state = DecoderState::DataMarker;
                            data_cursor = 0;
                        } else {
                            log += track_msg(track, "WARNING: expecting a mark 0x95 or 0x6A. Copy protection?");
                            gap_len = 0;
                            gap_index = dst.size();
                            state = DecoderState::PreGap;
                        }
                    } else
                    if (state == DecoderState::IndexMarker) {
                        if (aim_byte == 0x6A) {
                            state = DecoderState::SectorHeader;
                            idam_cursor = 0;
                            index_count++;
                        } else {
                            log += track_msg(track, "WARNING: expecting an INDEX mark 0x6A. Copy protection?");
                            gap_len = 0;
                            gap_index = dst.size();
                            state = DecoderState::PreGap;
                        }
                    } else
                    if (state == DecoderState::DataMarker) {
                        if (aim_byte == 0x95) {
                            state = DecoderState::SectorData;
                            data_cursor = 0;
                        } else {
                            log += track_msg(track, "WARNING: expecting a DATA mark 0x95. Copy protection?");
                            gap_len = 0;
                            gap_index = dst.size();
                            state = DecoderState::PreGap;
                        }
                    } else
                    if (state == DecoderState::SectorHeader) {
                        // A sector header is 4 bytes: Volume, Track, Sector, 0x5A
                        idam_cursor++;
                        if (idam_cursor == 4) {
                            if (aim_byte != 0x5A)
                                log += track_msg(track, "ERROR: the last INDEX byte must be 0x5A");
                            // A GAP begins here
                            gap_len = 0;
                            gap_index = dst.size();
                            state = DecoderState::PreGap;
                        }
                    } else
                    if (state == DecoderState::SectorData) {
                        if (data_cursor == SECTOR_LEN + 1 && aim_byte != 0x5A)
                            log += track_msg(track, "ERROR: the last DATA byte must be 0x5A");
                        if (data_cursor == SECTOR_LEN + 2) {
                            // A GAP begins here
                            gap_len = 0;
                            gap_index = dst.size();
                            state = DecoderState::PreGap;
                        }
                        data_cursor++;
                    }
                }
            }

            // End of GAP4
            if (gap_len > 0) {
                if (gap_index < dst.size()) {
                    restore_gap_filler(dst, gap_index);
                    dst[gap_index].cmd = gap_len;
                }
                if (gap_len > MINIMUM_GAP_LEN) gap_total_len += gap_len;
                gap_len = 0;
            }

            if (verbose) log += track_msg(track, std::to_string(index_count) + " sector(s), GAP total = " + std::to_string(gap_total_len)
                                                 + ", max GAP = " + std::to_string(max_gap_len));

            // Shifting the track data if an INDEX Start AIM command was found
            if (index_hole_ptr > 0) {
                if (verbose) log += track_msg(track, "INDEX Start command found, shifting the track content by "
                                                     + std::to_string(index_hole_ptr) + " MFM byte(s)");

                gap_len = 0;
                int shifts_after_last_gap = 0;
                for (int i = 0; i <= index_hole_ptr && !dst.empty(); i++) {
                    if (shifts_after_last_gap >= 1) shifts_after_last_gap++;

                    const AIMCell cell = dst.front();
                    dst.erase(dst.begin());
                    dst.push_back(cell);

                    if (cell.cmd > MINIMUM_GAP_LEN) {
                        gap_len = cell.cmd;
                        shifts_after_last_gap = 1;
                    }
                }

                // Correcting GAP1 and GAP4 length
                if (shifts_after_last_gap < gap_len) {
                    if (shifts_after_last_gap == 1)
                        log += track_msg(track, "ERROR: a GAP wrap of 1 byte will be confused with DESYNC");
                    dst[0].cmd = gap_len - shifts_after_last_gap;
                    dst[dst.size() - shifts_after_last_gap].cmd = shifts_after_last_gap;
                }
            }

            if (state != DecoderState::Desync)
                log += track_msg(track, "WARNING: PAYLOAD crossing the index hole. Copy protection?");

            return dst;
        }

        // Packs a track to exactly MFM_TRACK_CELLS cells, decreasing GAPs proportionally
        Result deflate_gaps(const AIMTrack & src, AIMTrack & dst, const int track, const int gap_total_len, std::string & log, const bool verbose)
        {
            const double deflation_rate = (gap_total_len > 0)
                ? (gap_total_len - (static_cast<int>(src.size()) - MFM_TRACK_CELLS)) / static_cast<double>(gap_total_len)
                : 0;
            double rounding_error = 0;

            // The last big GAP takes the accumulated rounding error
            int last_gap_index = -1;
            for (int i = static_cast<int>(src.size()) - 1; last_gap_index < 0 && i >= 0; i--)
                if (src[i].cmd > MINIMUM_GAP_LEN) last_gap_index = i;

            dst.clear();
            dst.reserve(MFM_TRACK_CELLS);

            DeflateState state = DeflateState::Payload;
            int gap_len = 0;
            int deflated_gap_len = 0;

            for (size_t i = 0; i < src.size(); i++) {
                AIMCell cell = src[i];

                // Only GAPs longer than MINIMUM_GAP_LEN are deflated
                if (cell.cmd > MINIMUM_GAP_LEN) {
                    gap_len = cell.cmd;
                    deflated_gap_len = static_cast<int>(gap_len * deflation_rate);
                    rounding_error += gap_len * deflation_rate - deflated_gap_len;
                    if (rounding_error > 1 || (rounding_error > 0 && last_gap_index == static_cast<int>(i))) {
                        deflated_gap_len++;
                        rounding_error--;
                    }
                    state = DeflateState::Gap;
                }

                if (cell.cmd > AIM_DESYNC) cell.cmd = 0;

                if (state == DeflateState::Gap) {
                    dst.push_back(cell);
                    gap_len--;
                    deflated_gap_len--;
                    if (deflated_gap_len == 0) state = DeflateState::Skip;
                    if (gap_len == 0) state = DeflateState::Payload;
                } else
                if (state == DeflateState::Skip) {
                    if (cell.data != GAP_FILLER)
                        log += track_msg(track, "WARNING: a GAP byte is not 0xAA. Payload in a GAP?");
                    gap_len--;
                    if (gap_len == 0 && i != src.size() - 1) state = DeflateState::Payload;
                } else {
                    dst.push_back(cell);
                }
            }

            if (verbose) log += track_msg(track, std::to_string(src.size()) + " -> " + std::to_string(dst.size()) + " byte(s)");

            if (dst.size() != MFM_TRACK_CELLS) {
                log += track_msg(track, "ERROR: packed to " + std::to_string(dst.size()) + " byte(s) instead of "
                                        + std::to_string(MFM_TRACK_CELLS) + ", GAP total = " + std::to_string(gap_total_len));
                return Result::error(ErrorCode::LoadDataCorrupt, QT_TRANSLATE_NOOP("errors", "Cannot pack a track to 6250 bytes"));
            }

            return Result::ok();
        }

        // Encodes a track as MFM, two MFM bytes per AIM cell, bits in the HFE order (LSB first)
        BYTES encode_mfm(const AIMTrack & track)
        {
            // MFM cell codes indexed by the previous and the current bit
            static const uint8_t mfm_code[2][2] = {{1 << 6, 2 << 6}, {0 << 6, 2 << 6}};

            BYTES mfm;
            mfm.reserve(track.size() * 2);

            uint8_t prev_bit = 0;

            for (size_t i = 0; i < track.size(); i++) {
                if (track[i].cmd == AIM_DESYNC) {
                    // A DESYNC is a missing clock pulse, it has no MFM encoding
                    mfm.push_back(0x22);
                    mfm.push_back(0x09);
                } else {
                    uint8_t data = track[i].data;
                    uint8_t first = 0;
                    uint8_t current = 0;

                    for (int bit_count = 1; bit_count <= 8; bit_count++) {
                        const uint8_t bit = (data & 0x80) >> 7;
                        data <<= 1;

                        if (bit_count == 5) {
                            // Advance to the next MFM byte
                            first = current;
                            current = 0;
                        } else {
                            current >>= 2;
                        }
                        current |= mfm_code[prev_bit][bit];
                        prev_bit = bit;
                    }

                    mfm.push_back(first);
                    mfm.push_back(current);
                }
            }

            return mfm;
        }

        void write_hfe(const std::vector<BYTES> & mfm, BYTES & out)
        {
            // HFE v2 padding, written instead of repeating the last byte of a track
            static const uint8_t hfe_padding[4] = {0x00, 0x06, 0xAA, 0xAA};

            const int cylinders = AIM_TRACKS / HFE_SIDES;
            const size_t track_len = mfm[0].size() * HFE_SIDES;
            const size_t blocks = (track_len + HFE_BLOCK - 1) / HFE_BLOCK;

            HXC_HFE_HEADER header;
            memset(&header, 0xFF, sizeof(header));

            std::strncpy(reinterpret_cast<char *>(&header.HEADERSIGNATURE[0]), "HXCPICFE", 8);
            header.formatrevision = 1;                  // HFE v2
            header.number_of_track = cylinders;
            header.number_of_side = HFE_SIDES;
            header.track_encoding = ISOIBM_MFM_ENCODING;
            header.bitRate = HFE_BITRATE;
            header.floppyRPM = 0;
            header.floppyinterfacemode = GENERIC_SHUGGART_DD_FLOPPYMODE;
            header.write_protected = 0x00;
            header.track_list_offset = 1;               // the track list follows the header

            uint8_t * ptr = reinterpret_cast<uint8_t*>(&header);
            out.insert(out.end(), ptr, ptr + sizeof(header));
            out.insert(out.end(), HFE_BLOCK - sizeof(header), 0xFF);

            // Track list
            HXC_HFE_TRACK lut;
            ptr = reinterpret_cast<uint8_t*>(&lut);
            uint16_t offset = 2;
            for (int cylinder = 0; cylinder < cylinders; cylinder++) {
                lut.offset = offset;
                lut.track_len = static_cast<uint16_t>(track_len);
                out.insert(out.end(), ptr, ptr + sizeof(lut));
                offset += blocks;
            }
            out.insert(out.end(), HFE_BLOCK - sizeof(lut) * cylinders, 0xFF);

            // Track data, sides alternating in 256 byte half blocks
            for (int cylinder = 0; cylinder < cylinders; cylinder++) {
                size_t position[HFE_SIDES] = {0, 0};

                for (size_t i = 0; i < blocks * HFE_BLOCK; ) {
                    const int side = (i & 0x100) ? 1 : 0;
                    const BYTES & data = mfm[cylinder * HFE_SIDES + side];

                    if (position[side] < data.size()) {
                        out.push_back(data[position[side]++]);
                        i++;
                    } else {
                        out.insert(out.end(), hfe_padding, hfe_padding + sizeof(hfe_padding));
                        i += sizeof(hfe_padding);
                    }
                }
            }
        }

    } // namespace

    Result AIM2HFEConverter::convert(const BYTES & in, BYTES & out, std::string & log, bool verbose)
    {
        if (in.size() < static_cast<size_t>(AIM_TRACKS) * AIM_TRACK_CELLS * 2)
            return Result::error(ErrorCode::LoadSizeMismatch, QT_TRANSLATE_NOOP("errors", "File too small"));

        // Reading AIM
        std::vector<AIMTrack> tracks(AIM_TRACKS);
        for (int track = 0; track < AIM_TRACKS; track++) {
            const uint8_t * ptr = in.data() + static_cast<size_t>(track) * AIM_TRACK_CELLS * 2;
            tracks[track].resize(AIM_TRACK_CELLS);
            for (int i = 0; i < AIM_TRACK_CELLS; i++)
                tracks[track][i] = {ptr[i*2], ptr[i*2 + 1]};
        }

        // Expanding DESYNC and measuring GAPs
        if (verbose) log += "Expanding DESYNC\n";
        std::vector<int> gap_total_len(AIM_TRACKS);
        for (int track = 0; track < AIM_TRACKS; track++)
            tracks[track] = expand_desync(tracks[track], track, gap_total_len[track], log, verbose);

        // Deflating GAPs to fit a track into 6250 bytes
        if (verbose) log += "Deflating GAPs\n";
        for (int track = 0; track < AIM_TRACKS; track++) {
            AIMTrack deflated;
            const Result res = deflate_gaps(tracks[track], deflated, track, gap_total_len[track], log, verbose);
            if (!res) return res;
            tracks[track] = deflated;
        }

        // Encoding MFM
        if (verbose) log += "Encoding MFM\n";
        std::vector<BYTES> mfm(AIM_TRACKS);
        for (int track = 0; track < AIM_TRACKS; track++)
            mfm[track] = encode_mfm(tracks[track]);

        // Creating HFE
        if (verbose) log += "Creating HFE\n";
        out.clear();
        write_hfe(mfm, out);

        return Result::ok();
    }
}
