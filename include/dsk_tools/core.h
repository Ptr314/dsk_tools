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
    // IBM MFM, 250 kbit/s, 300 rpm, 6250 bytes a turn (the К1801ВП1-128 of the
    // БК, the УК-НЦ and the DVK MY): a 4E gap, 12 zeros and three A1 sync
    // bytes before a field; the ID field FE, track, side, sector, size code,
    // CRC; the data field FB (F8 when deleted), the sector, CRC. CRC-16-CCITT
    // preset FFFF from the first A1.
    //
    // IBM 3740 FM, 8", 250 kbit/s, 360 rpm, 5208 bytes a turn (RX01, the DVK
    // DX): FF gaps, 6 zeros before a mark, the index mark FC, ID FE and data
    // FB/F8 written with a clock of their own.
    //
    // DVK MX, FM, 1562 words a turn (one per 128 us), low byte first: 8 zero
    // words, the sync word 000363, the track number, 11 sectors of 128 words
    // each followed by the sum of its words, three words 0101400 + track * 2 +
    // side. The controller knows no sectors: the layout is the RT-11 driver's.
    //
    // Reading places a sector by the number in its ID field; deleted is a
    // bit per sector with the deleted data mark
    #define IBM_MFM_TRACK_BYTES     6250
    #define IBM_FM_TRACK_BYTES      5208
    #define DVK_MX_TRACK_WORDS      1562
    #define DVK_MX_SYNC_WORD        0000363
    #define DVK_MX_SECTORS          11
    #define DVK_MX_SECTOR_SIZE      256

    int ibm_size_code(int sector_size);
    void ibm_mfm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                              const uint8_t * flat, uint16_t deleted);
    bool ibm_mfm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                            uint16_t & deleted, int & found);
    void ibm_mfm_find_marks(const uint8_t * data, size_t len, BYTES & special);
    void ibm_fm_format_track(BYTES & data, BYTES & special, int track, int side, int sectors, int sector_size,
                             const uint8_t * flat, uint16_t deleted);
    bool ibm_fm_read_track(const uint8_t * data, size_t len, int sectors, int sector_size, uint8_t * out,
                           uint16_t & deleted, int & found);
    void ibm_fm_find_marks(const uint8_t * data, size_t len, BYTES & special);
    // The data mark of the sector with this track and number in its ID field:
    // the offset of the mark byte or -1. size is the sector size by the ID
    int ibm_fm_find_sector(const uint8_t * data, size_t len, int track, int sector, bool & deleted, int & size);
    // The sector into its place: the mark (FB or F8), the data and the CRC
    void ibm_fm_put_sector(uint8_t * data, size_t len, int mark, const uint8_t * src, int size, bool deleted);
    void dvk_mx_format_track(BYTES & data, int track, int side, const uint8_t * flat);
    bool dvk_mx_read_track(const uint8_t * data, size_t len, uint8_t * out);

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
        std::vector<BYTES> cells;           // track * sides + side
    };
    Result hfe_read(const BYTES & file, HfeImage & img);
    void hfe_write(const HfeImage & img, BYTES & file);

    // The RT-11 disks of the DVK, the БК and the УК-НЦ (TYPE_RT11:*) as bit
    // cells. kind: 0 - not one of them, 1 - IBM MFM (MY, MZ), 2 - IBM 3740 FM
    // (DX), 3 - DVK MX. Writing builds the tracks of a flat image (sectors in
    // order, both sides of a cylinder in turn) with hfe_params' rate and rpm;
    // reading places the sectors it finds and lists the missing ones
    int rt11_track_kind(const std::string & type_id);
    void rt11_hfe_params(int kind, HfeImage & img);
    void rt11_track_cells(int kind, int track, int side, int sectors, int sector_size, const uint8_t * flat, BYTES & cells);
    // The sectors of one track: false when none was found; missing gets a bit
    // per sector (by its number - 1) that is not on the track
    bool rt11_track_sectors(int kind, const BYTES & cells, int sectors, int sector_size, uint8_t * out, uint32_t & missing);
    // The kind of the disk in an HFE, by its first track; 0 when it is none of them
    int rt11_hfe_kind(const HfeImage & img);

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
