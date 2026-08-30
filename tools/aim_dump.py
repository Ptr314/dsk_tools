#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
#
# Run length dump of an AIM track. A cell is printed as DD[/CC][*N], where DD is
# the data byte, CC the AIM command (omitted when zero) and N the repeat count.
#
# Usage: aim_dump.py <file.aim> <track> [first cell] [runs to print]

import sys

from aim_common import CELLS, read_track


def main(argv):
    if len(argv) < 3:
        sys.exit("Usage: %s <file.aim> <track> [first cell] [runs to print]" % argv[0])

    file_name = argv[1]
    track = int(argv[2])
    first = int(argv[3]) if len(argv) > 3 else 0
    count = int(argv[4]) if len(argv) > 4 else 60

    data, cmd = read_track(file_name, track)

    out = []
    i = first
    while i < CELLS and len(out) < count:
        j = i
        while j < CELLS and data[j] == data[i] and cmd[j] == cmd[i]:
            j += 1
        out.append("[%d]%02X%s%s" % (i, data[i],
                                     "/%02X" % cmd[i] if cmd[i] else "",
                                     "*%d" % (j - i) if j - i > 1 else ""))
        i = j

    print(" ".join(out))


if __name__ == "__main__":
    main(sys.argv)
