// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: The part of the library that needs no loaders, images, file systems
// or viewers: definitions, string and host file helpers, track encodings.
// Built alone as the dsk_tools_core target; dsk_tools.h includes it

#pragma once

#include <string>

// Named from this directory: MSVC looks up a quoted include in the directories of every
// including file too, and a program with a utils.h of its own would get that one
#include "../../src/definitions.h"
#include "../../src/utils.h"
#include "../../src/bit_enums.h"
#include "../../src/host_helpers.h"

namespace dsk_tools {

    BYTES code44(const BYTES & buffer);
    BYTES decode44(const BYTES & buffer);
    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    bool decode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    uint16_t encode_agat_MFM_byte(uint8_t data, uint8_t &last_byte);
    uint8_t decode_agat_MFM_byte(uint8_t data);
    void encode_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t & last_byte);
    uint8_t encode_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t & last_byte);
    void decode_agat_mfm_data(BYTES &out, const BYTES & in);
    int detect_agat_sector_size(const BYTES & in);
    Result decode_agat_840_track(BYTES &out, const BYTES & in, const int sectors = 21, const int sector_size = 256);
    Result decode_agat_840_image(BYTES &out, const BYTES & in);

    Result load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len);
    Result decode_agat_140_image(BYTES &out, const BYTES & in, const int track_len);

    // Whole tracks as the controllers lay them down: the decoded byte stream of
    // one turn, and a flag per byte for the bytes written without their usual
    // clock (special) - which is what the cell codecs below take.
    //
    // IBM MFM (System/34), 250 kbit/s, 300 rpm, 6250 bytes a turn: the
    // WD1793/КР1818ВГ93 of the PC, Atari ST, Korvet, Orion and Vector, the
    // К1801ВП1-128 of the БК, the УК-НЦ and the DVK MY. A 4E gap, 12 zeros and
    // three A1 sync bytes before a field; the index mark C2 C2 C2 FC; the ID
    // field FE, track, side, sector, size code, CRC; the data field FB (F8
    // when deleted), the sector, CRC. CRC-16-CCITT preset FFFF from the first A1.
    //
    // IBM 3740 FM, 8", 250 kbit/s, 360 rpm, 5208 bytes a turn (RX01, the DVK
    // DX): FF gaps, 6 zeros before a mark, the index mark FC, ID FE and data
    // FB/F8 written with a clock of their own; the CRC starts at the mark.
    //
    // DVK MX, FM, 1562 words a turn (one per 128 us), low byte first: 8 zero
    // words, the sync word 000363, the track number, 11 sectors of 128 words
    // each followed by the sum of its words, three words 0101400 + track * 2 +
    // side. The controller knows no sectors: the layout is the RT-11 driver's.
    #define IBM_MFM_TRACK_BYTES     6250
    #define IBM_FM_TRACK_BYTES      5208
    #define DVK_MX_TRACK_WORDS      1562
    #define DVK_MX_SYNC_WORD        0000363
    #define DVK_MX_SECTORS          11
    #define DVK_MX_SECTOR_SIZE      256

    // A sector of an IBM track as it was found: its ID field and, when there is
    // one, the data field that follows it
    struct IbmSectorField {
        size_t  id_pos;                     // the ID mark FE in the track bytes
        size_t  data_pos;                   // the data mark FB or F8; IBM_NO_DATA when missing
        uint8_t track;
        uint8_t side;
        uint8_t sector;
        uint8_t size_code;
        bool    id_crc_ok;
        bool    data_crc_ok;
        bool    deleted;
        int size() const { return 128 << (size_code & 7); }
    };
    static const size_t IBM_NO_DATA = static_cast<size_t>(-1);

    struct IbmTrackFields {
        std::vector<size_t>         index_marks;    // FC of the index marks
        std::vector<IbmSectorField> sectors;        // in their order on the track
    };

    // The fields of a decoded track. special is a flag per byte as the cell
    // decoders give it; without it (nullptr) the marks are told by the bytes
    // alone: A1 A1 A1 before FE/FB/F8 and C2 C2 C2 before FC in MFM, a zero
    // before the mark in FM. Anything between the fields - a write splice, an
    // unformatted end of the track - is skipped
    void ibm_track_fields(const uint8_t * data, size_t len, const uint8_t * special, bool mfm, IbmTrackFields & out);

    // How a track is formatted. A gap left at -1 takes the IBM value for the
    // encoding and the sector size; gap3 is then narrowed to fit the turn
    struct IbmTrackFormat {
        bool      mfm           = true;
        int       track_bytes   = IBM_MFM_TRACK_BYTES;
        int       sectors       = 10;
        int       sector_size   = 512;
        int       first_sector  = 1;
        TrackGaps gaps;
    };
    // A whole track out of the sectors of a flat image (sector n at n * size),
    // deleted - a bit per sector for F8. false when the sectors do not fit
    bool ibm_format_track(const IbmTrackFormat & fmt, int track, int side, const uint8_t * flat, uint32_t deleted,
                          BYTES & data, BYTES & special);
    int ibm_size_code(int sector_size);

    // The track the RT-11 drivers of the БК, the УК-НЦ and the DVK MY write
    // (no index mark, 42/22/36 byte gaps) and the IBM 3740 track of the DX
    void ibm_mfm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                              const uint8_t * flat, uint16_t deleted);
    void ibm_fm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                             const uint8_t * flat, uint16_t deleted);
    // Sectors placed by the number in their ID field (1..sectors) whatever
    // their CRC says; deleted is a bit per sector with the deleted data mark,
    // found the number of sectors placed. false when none was
    bool ibm_mfm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                            uint16_t & deleted, int & found);
    bool ibm_fm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                           uint16_t & deleted, int & found);
    // The sync bytes (MFM) and the marks (FM) as flags, for the cell encoders
    void ibm_mfm_find_marks(const uint8_t * data, size_t len, BYTES & special);
    void ibm_fm_find_marks(const uint8_t * data, size_t len, BYTES & special);
    // The data mark of the sector with this track and number in its ID field:
    // the offset of the mark byte or -1. size is the sector size by the ID
    int ibm_fm_find_sector(const uint8_t * data, size_t len, int track, int sector, bool & deleted, int & size);
    // The sector into its place: the mark (FB or F8), the data and the CRC
    void ibm_fm_put_sector(uint8_t * data, size_t len, int mark, const uint8_t * src, int size, bool deleted);

    void dvk_mx_format_track(BYTES & data, int track, int side, const uint8_t * flat);
    bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out);
    // The same, bad getting a bit per sector whose sum of words does not match
    bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out, uint32_t & bad);

    // HFE (HxC Floppy Emulator, v1): the bit cells of every track and side as
    // they pass the head, the first cell in bit 0 of the first byte. A file of
    // the HxC software and of drive emulators such as Gotek (FlashFloppy)
    struct HfeImage {
        int tracks = 0;
        int sides = 0;
        uint16_t bitrate = 250;             // data rate, kbit/s: the cells go at twice that
        uint16_t rpm = 300;
        uint8_t encoding = ISOIBM_MFM_ENCODING;
        uint8_t interface_mode = 0x07;      // GENERIC_SHUGART_DD_FLOPPYMODE
        bool write_allowed = true;
        std::vector<BYTES> cells;           // track * sides + side
    };
    Result hfe_read(const BYTES & file, HfeImage & img);
    void hfe_write(const HfeImage & img, BYTES & file);

    // Disks as bit cells by their TrackLayout (DiskFormatParams::layout): the
    // geometry, the data rate and the rpm come from the format, sectors are
    // numbered from sector_base and laid out in order, both sides of a
    // cylinder in turn
    TrackLayout track_layout_by_name(const std::string & name);     // "ibm-mfm", "ibm-fm", "dvk-mx"
    std::string track_layout_name(TrackLayout layout);
    // The data rate, the rpm and the HFE encoding of the layout where the
    // format leaves them at 0: 250 kbit/s (125 for MX), 300 rpm (360 for FM)
    void track_layout_defaults(DiskFormatParams & format);
    // Bytes in one turn at the format's data rate and rpm
    int track_bytes(const DiskFormatParams & format);
    void track_hfe_params(const DiskFormatParams & format, HfeImage & img);
    bool track_to_cells(const DiskFormatParams & format, int track, int side, const uint8_t * flat, BYTES & cells);

    // What reading a track found, a bit per sector (by its number - sector_base)
    struct TrackStatus {
        uint32_t missing = 0;               // not on the track, or with a broken ID field
        uint32_t bad_crc = 0;               // there, but the data does not match its CRC (sum for MX)
        uint32_t deleted = 0;               // written with the deleted data mark
    };
    // The decoded bytes of a track and the fields in them (IBM layouts only)
    void track_decode(const DiskFormatParams & format, const BYTES & cells, BYTES & data, IbmTrackFields & fields);
    // The sectors of a track into out (sectors * sector_size, left as it is where
    // a sector is missing). false when no sector was found at all
    bool track_from_cells(const DiskFormatParams & format, const BYTES & cells, uint8_t * out, TrackStatus & status);
    // The whole disk of an HFE into a flat image; bad gets bad_sector_key(head,
    // track, sector number) of every sector missing or failing its CRC
    void hfe_to_flat(const HfeImage & img, const DiskFormatParams & format, BYTES & out, BadSectorTable * bad = nullptr);
    // The layout and the track geometry of an HFE told by its first tracks:
    // layout, heads, tracks, sectors, sector_size, sector_base, bitrate, rpm.
    // false when it is none of the layouts above
    bool hfe_probe_format(const HfeImage & img, DiskFormatParams & format);

    // Bytes of a track and its bit cells, 16 cells a byte. special - a flag
    // per byte: in MFM a sync byte with a missing clock (A1 is 4489, C2 is
    // 5224), in FM an address mark with a clock of its own (FC with D7, FE,
    // FB and F8 with C7). Decoding takes the byte grid anew at every mark and
    // gives len bytes
    void mfm_encode(const uint8_t * data, const uint8_t * special, size_t len, BYTES & cells);
    void mfm_decode(const BYTES & cells, size_t len, BYTES & data, BYTES & special);
    void fm_encode(const uint8_t * data, const uint8_t * special, size_t len, BYTES & cells);
    void fm_decode(const BYTES & cells, size_t len, BYTES & data, BYTES & special);

} // namespace
