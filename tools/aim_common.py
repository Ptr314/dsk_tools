#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
#
# Shared AIM parsing code for the aim_*.py tools.
#
# An AIM track is 6464 cells, each one being a data byte and an AIM command.
# A standard Agat 840K sector is
#   DESYNC 95 6A | volume track sector 5A | gap | DESYNC 6A 95 | 256 bytes | crc 5A | gap
# The dump is slightly longer than one revolution, so a track is treated as a
# ring: its last field wraps to the beginning and the first cells may be a
# re-read of the last ones.

TRACKS = 160
CELLS = 6464
SECTORS = 21
SECTOR_SIZE = 256

# AIM commands
DESYNC = 0x01
TRACK_END = 0x02
INDEX_START = 0x03
INDEX_END = 0x13
DESYNC_ALT = 0x80
DESYNC_BOTH = 0x81      # some dumps set both DESYNC bits at once

ADDRESS_MARK = (0x95, 0x6A)
DATA_MARK = (0x6A, 0x95)
EPILOGUE = 0x5A

# DESYNC 95 6A | volume track sector 5A
ADDRESS_FIELD_CELLS = 7
# DESYNC 6A 95 | 256 bytes | crc 5A
DATA_FIELD_CELLS = 3 + SECTOR_SIZE + 2


def is_desync(command):
    return command in (DESYNC, DESYNC_ALT, DESYNC_BOTH)


def is_filler(value):
    """A gap byte: the filler itself, an unread cell or a part of a DESYNC."""
    return value in (0xAA, 0x00, 0xA4, 0xFF)


def crc_agat(data):
    """The Agat 840 data checksum: an 8 bit sum with the carry added back."""
    crc = 0
    for byte in data:
        if crc > 0xFF:
            crc = (crc + 1) & 0xFF
        crc += byte
    return crc & 0xFF


def crc_xor(data):
    crc = 0
    for byte in data:
        crc ^= byte
    return crc


def read_track(file_name, track):
    """Returns the (data bytes, AIM commands) of one track."""
    with open(file_name, "rb") as f:
        f.seek(track * CELLS * 2)
        buf = f.read(CELLS * 2)
    if len(buf) < CELLS * 2:
        raise SystemExit("%s: track %d is out of the file" % (file_name, track))
    return bytearray(buf[0::2]), bytearray(buf[1::2])


def read_aim(file_name):
    """Returns the (data bytes, AIM commands) of the whole image, track by track."""
    with open(file_name, "rb") as f:
        buf = f.read()
    if len(buf) < TRACKS * CELLS * 2:
        raise SystemExit("%s: the file is smaller than %d bytes" % (file_name, TRACKS * CELLS * 2))
    return bytearray(buf[0::2]), bytearray(buf[1::2])


def parse_track(data, cmd):
    """Splits a track into address and data fields.

    data and cmd are the cells of a single track. Returns (fields, anomalies,
    track_end), where a field is a dict with

        kind  'A' for an address field, 'D' for a data field
        pos   the cell of its DESYNC
        gap   (from, to) cells of the gap before it
        A:    vol, trk, sec, epi
        D:    sum, epi, agat, xor, inner (DESYNCs inside the data), data
    """
    ring_d = data + data          # a field may cross the end of the track
    ring_c = cmd + cmd

    fields = []
    anomalies = []
    track_end = None
    gap_from = None
    p = 0

    while p < CELLS:
        command = cmd[p]

        if command == TRACK_END:
            track_end = p
            break

        if command in (INDEX_START, INDEX_END):
            p += 1
            continue

        if command != 0 and not is_desync(command):
            anomalies.append("unknown AIM command $%02X at %d" % (command, p))
            p += 1
            continue

        if is_desync(command):
            if gap_from is None:
                gap_from = p

            mark = (ring_d[p + 1], ring_d[p + 2])

            if mark == ADDRESS_MARK:
                fields.append({"kind": "A", "pos": p, "gap": (gap_from, p),
                               "vol": ring_d[p + 3], "trk": ring_d[p + 4],
                               "sec": ring_d[p + 5], "epi": ring_d[p + 6]})
                p += ADDRESS_FIELD_CELLS
                gap_from = p
                continue

            if mark == DATA_MARK:
                end = p + 3 + SECTOR_SIZE
                payload = ring_d[p + 3:end]
                fields.append({"kind": "D", "pos": p, "gap": (gap_from, p),
                               "sum": ring_d[end], "epi": ring_d[end + 1],
                               "agat": crc_agat(payload), "xor": crc_xor(payload),
                               "inner": [i for i in range(p + 3, end + 2) if is_desync(ring_c[i])],
                               "data": bytes(payload)})
                p = end + 2
                gap_from = p
                continue

            anomalies.append(
                "DESYNC at %d is followed by $%02X $%02X, not by an address (95 6A) or a data (6A 95) mark"
                % (p, mark[0], mark[1]))

        p += 1

    return fields, anomalies, track_end


def pair_sectors(fields):
    """Pairs every address field with the data field that follows it.

    The last address field of a track may own the data field at its beginning.
    Returns (pairs, anomalies).
    """
    pairs = []
    paired = set()
    anomalies = []

    for k, field in enumerate(fields):
        if field["kind"] != "A":
            continue
        following = fields[k + 1] if k + 1 < len(fields) else None
        if following is None and fields and fields[0]["kind"] == "D":
            following = fields[0]
        if following is not None and following["kind"] == "D":
            pairs.append((field, following))
            paired.add(following["pos"])
        else:
            anomalies.append("address field at %d (sector %d) is not followed by a data field"
                             % (field["pos"], field["sec"]))

    for field in fields[1:]:      # field 0 may be the tail of the wrapped one
        if field["kind"] == "D" and field["pos"] not in paired:
            anomalies.append("data field at %d has no address field before it" % field["pos"])

    return pairs, anomalies


def runs(data, cmd, first, last):
    """Run length dump of a cell range, as DD[/CC][*N]."""
    out = []
    i = first
    while i < last:
        j = i
        while j < last and data[j] == data[i] and cmd[j] == cmd[i]:
            j += 1
        out.append("%02X%s%s" % (data[i],
                                 "/%02X" % cmd[i] if cmd[i] else "",
                                 "*%d" % (j - i) if j - i > 1 else ""))
        i = j
    return " ".join(out)
