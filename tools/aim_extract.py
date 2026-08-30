#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
#
# Builds a raw 160*21*256 image out of an AIM file, placing every sector by the
# number taken from its address field. An independent implementation, useful as
# a reference when checking LoaderAIM or a conversion.
#
# Usage: aim_extract.py <file.aim> <out.dsk>

import sys

from aim_common import CELLS, SECTORS, SECTOR_SIZE, TRACKS, pair_sectors, parse_track, read_aim


def main(argv):
    if len(argv) < 3:
        sys.exit("Usage: %s <file.aim> <out.dsk>" % argv[0])

    file_name, out_name = argv[1], argv[2]
    data, cmd = read_aim(file_name)

    image = bytearray(TRACKS * SECTORS * SECTOR_SIZE)
    found = 0

    for track in range(TRACKS):
        base = track * CELLS
        fields, _, _ = parse_track(data[base:base + CELLS], cmd[base:base + CELLS])
        pairs, _ = pair_sectors(fields)

        for address, field in pairs:
            sector = address["sec"]
            if 0 <= sector < SECTORS:
                out_p = (track * SECTORS + sector) * SECTOR_SIZE
                image[out_p:out_p + SECTOR_SIZE] = field["data"]
                found += 1

    with open(out_name, "wb") as f:
        f.write(image)

    print("%d sector(s) written to %s" % (found, out_name))


if __name__ == "__main__":
    main(sys.argv)
