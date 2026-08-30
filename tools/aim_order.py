#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
#
# Physical order of the sectors on every track of an AIM file. An AIM dump starts
# at an arbitrary angle, so a track read as 5,6,...,20,0,...,4 is just a rotation
# of the normal order and is perfectly fine. A track that is not a rotation of
# 0..20 was written with a non-standard layout.
#
# Usage: aim_order.py <file.aim> [track ...]      (no track: summary of the disk)

import os
import sys

from aim_common import CELLS, SECTORS, TRACKS, parse_track, read_aim


def sector_order(data, cmd, track):
    base = track * CELLS
    fields, _, _ = parse_track(data[base:base + CELLS], cmd[base:base + CELLS])
    return [f["sec"] for f in fields if f["kind"] == "A"]


def rotation_of(order):
    """The k for which the order is 0..20 rotated by k, or None."""
    if len(order) != SECTORS:
        return None
    for k in range(SECTORS):
        if all(order[j] == (k + j) % SECTORS for j in range(SECTORS)):
            return k
    return None


def main(argv):
    if len(argv) < 2:
        sys.exit("Usage: %s <file.aim> [track ...]" % argv[0])

    file_name = argv[1]
    tracks = [int(a) for a in argv[2:]]
    summary = not tracks
    if summary:
        tracks = list(range(TRACKS))

    data, cmd = read_aim(file_name)
    rotations = {}
    odd = []

    for track in tracks:
        order = sector_order(data, cmd, track)
        rotation = rotation_of(order)
        if summary:
            if rotation is None:
                odd.append((track, order))
            else:
                rotations[rotation] = rotations.get(rotation, 0) + 1
        else:
            print("track %3d: %-70s %s" % (track, ",".join(str(s) for s in order),
                                           "(rotation %d)" % rotation if rotation is not None else "NOT A ROTATION"))

    if summary:
        print("%s: %d track(s) are a rotation of 0..20, %d are not"
              % (os.path.basename(file_name), len(tracks) - len(odd), len(odd)))
        print("   rotations: %s" % ", ".join("%d x%d" % (k, rotations[k]) for k in sorted(rotations)))
        for track, order in odd:
            print("   track %3d: %s" % (track, ",".join(str(s) for s in order)))


if __name__ == "__main__":
    main(sys.argv)
