#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
#
# Scans an Agat 840K AIM dump and reports everything that does not fit the
# standard track layout: non-standard epilogues, checksum mismatches, duplicated
# and missing sectors, DESYNC marks in unexpected places, unknown AIM commands
# and payload written into the gaps.
#
# Usage: aim_scan.py <file.aim> [more files ...]

import os
import sys

from aim_common import (CELLS, EPILOGUE, SECTORS, TRACKS, is_filler,
                        pair_sectors, parse_track, read_aim, runs)

WRAP = 6000     # two copies of a sector further apart than this are the ring overlap
COMMON = 20     # a gap pattern seen more often than this belongs to the disk itself


def by_count(counters):
    """Keys ordered by their count, the equal ones alphabetically, so that the
    report of the same image is always the same."""
    return sorted(counters, key=lambda k: (-counters[k], k))


def check_sectors(pairs, track, anomalies, volumes, track_field):
    """Per sector checks. Returns (sectors by number, bad checksums, XOR checksums)."""
    seen = {}
    crc_bad = 0
    crc_xor = 0

    for address, field in pairs:
        sector = address["sec"]
        seen.setdefault(sector, []).append((address, field))

        key = "equal to the sector number" if address["vol"] == sector else "$%02X" % address["vol"]
        volumes[key] = volumes.get(key, 0) + 1
        if address["trk"] == track:
            key = "AIM track number"
        elif address["trk"] == track // 2:
            key = "cylinder number"
        else:
            key = "other"
        track_field[key] = track_field.get(key, 0) + 1

        if address["epi"] != EPILOGUE:
            anomalies.append("sector %d at %d: address epilogue $%02X instead of $5A"
                             % (sector, address["pos"], address["epi"]))
        if sector > 20:
            anomalies.append("sector %d at %d: sector number is out of the 0..20 range"
                             % (sector, address["pos"]))
        if field["sum"] != field["agat"]:
            crc_bad += 1
            if field["sum"] == field["xor"]:
                crc_xor += 1
                anomalies.append("sector %d at %d: checksum $%02X is an XOR of the data, not the Agat sum $%02X"
                                 % (sector, field["pos"], field["sum"], field["agat"]))
            else:
                anomalies.append("sector %d at %d: data checksum $%02X, calculated $%02X"
                                 % (sector, field["pos"], field["sum"], field["agat"]))
        if field["epi"] != EPILOGUE:
            anomalies.append("sector %d at %d: data epilogue $%02X instead of $5A"
                             % (sector, field["pos"], field["epi"]))
        if field["inner"]:
            anomalies.append("sector %d at %d: DESYNC inside the data field at %s"
                             % (sector, field["pos"], ",".join(str(i) for i in field["inner"])))

    return seen, crc_bad, crc_xor


def check_duplicates(seen, anomalies):
    for sector in sorted(seen):
        copies = seen[sector]
        if len(copies) < 2:
            continue
        delta = copies[-1][0]["pos"] - copies[0][0]["pos"]
        same = copies[0][1]["data"] == copies[-1][1]["data"]
        if delta > WRAP and same:
            continue        # the same sector re-read at the ring overlap
        where = " | ".join(
            "at %d (volume $%02X, track %d, epilogue $%02X, checksum %s)"
            % (address["pos"], address["vol"], address["trk"], address["epi"],
               "ok" if field["sum"] == field["agat"] else "$%02X != $%02X" % (field["sum"], field["agat"]))
            for address, field in copies)
        anomalies.append("DUPLICATE sector %d: %s%s"
                         % (sector, where, ", data identical" if same else ", DATA DIFFERENT"))


def scan_file(file_name):
    data, cmd = read_aim(file_name)

    gap_signatures = {}
    gaps = []
    per_track = {}
    volumes = {}
    track_field = {}
    commands = {}
    notes = []
    total = crc_bad = crc_xor = 0

    for track in range(TRACKS):
        base = track * CELLS
        track_d = data[base:base + CELLS]
        track_c = cmd[base:base + CELLS]

        for command in track_c:
            if command:
                commands[command] = commands.get(command, 0) + 1

        fields, anomalies, track_end = parse_track(track_d, track_c)
        pairs, pairing_anomalies = pair_sectors(fields)
        anomalies += pairing_anomalies

        seen, bad, xor = check_sectors(pairs, track, anomalies, volumes, track_field)
        total += len(pairs)
        crc_bad += bad
        crc_xor += xor
        check_duplicates(seen, anomalies)

        missing = [s for s in range(SECTORS) if s not in seen]
        if missing:
            anomalies.append("missing sector(s): " + ",".join(str(s) for s in missing))
        if len(pairs) != SECTORS:
            anomalies.append("%d sector(s) instead of 21" % len(pairs))
        if track_end is not None:
            anomalies.append("end of track command at %d, the rest of the AIM track is padding" % track_end)
        per_track[len(pairs)] = per_track.get(len(pairs), 0) + 1

        for field in fields:
            first, last = field["gap"]
            if first is None or last <= first:
                continue
            signature = " ".join("%02X" % track_d[p] for p in range(first, last) if not is_filler(track_d[p]))
            if not signature:
                signature = "(filler only)"
            gap_signatures[signature] = gap_signatures.get(signature, 0) + 1
            gaps.append((signature, track, first, last,
                         "" if signature == "(filler only)" else runs(track_d, track_c, first, last)))

        if anomalies:
            notes.append((track, anomalies))

    # ---- the report
    print("########## %s" % os.path.basename(file_name))
    print("   %d sector(s), %d with a bad checksum (%d of them an XOR) | sectors per track: %s"
          % (total, crc_bad, crc_xor, ", ".join("%d x%d" % (n, per_track[n]) for n in sorted(per_track))))
    print("   volume field: %s"
          % ", ".join("%s x%d" % (k, volumes[k]) for k in by_count(volumes)))
    print("   track field: %s | AIM commands: %s"
          % (", ".join("%s x%d" % (k, track_field[k]) for k in by_count(track_field)),
             ", ".join("$%02X x%d" % (c, commands[c]) for c in sorted(commands))))

    order = by_count(gap_signatures)
    print("   gap contents: %s"
          % " | ".join("[%s] x%d" % (s, gap_signatures[s]) for s in order[:3]))

    for signature in order:
        if signature == "(filler only)" or gap_signatures[signature] > COMMON:
            continue
        print("   [gap] [%s], %d time(s):" % (signature, gap_signatures[signature]))
        examples = [g for g in gaps if g[0] == signature]
        for _, track, first, last, dump in examples[:3]:
            print("         track %d, cells %d..%d: %s" % (track, first, last, dump))

    for track, anomalies in notes:
        print("   [!!] track %d" % track)
        for line in anomalies:
            print("        " + line)


def main(argv):
    if len(argv) < 2:
        sys.exit("Usage: %s <file.aim> [more files ...]" % argv[0])
    for file_name in argv[1:]:
        scan_file(file_name)


if __name__ == "__main__":
    main(sys.argv)
