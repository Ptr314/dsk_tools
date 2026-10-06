#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
# Description: raw image -> HFE -> raw image through fddconv, for every image found
#
#   tools/hfe_roundtrip.sh <fddconv> <directory or image> ...
#
# For every raw image that fddconv recognises and can open, the HFE written
# from it must be detected as the same type, list the same files and give
# back the same sectors. Images whose filesystem does not open as they are
# are counted apart: they say nothing about HFE.

set -u
fddconv=$1
shift
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

total=0; ok=0; failed=0; skipped=0
while IFS= read -r -d '' f; do
    total=$((total+1))
    type=$("$fddconv" "$f" -l -v 2>/dev/null | grep -a "Type =" | head -1 | sed 's/.*= //')
    if [ -z "$type" ] || ! "$fddconv" "$f" -l > "$work/src.txt" 2>&1; then
        skipped=$((skipped+1)); continue
    fi
    rm -f "$work/out.hfe" "$work/back.img"
    if ! "$fddconv" "$f" -o "$work/out.hfe" > "$work/w.txt" 2>&1; then
        echo "WRITE FAILED [$type] $f: $(cat "$work/w.txt")"; failed=$((failed+1)); continue
    fi
    hfe_type=$("$fddconv" "$work/out.hfe" -l -v 2>&1 | grep -a "Type =" | head -1 | sed 's/.*= //')
    "$fddconv" "$work/out.hfe" -l > "$work/hfe.txt" 2>&1
    "$fddconv" "$work/out.hfe" -o "$work/back.img" > /dev/null 2>&1
    same_list=0; cmp -s "$work/src.txt" "$work/hfe.txt" && same_list=1
    n=$(wc -c < "$work/back.img" 2>/dev/null || echo 0)
    same_data=0; [ "$n" -gt 0 ] && cmp -s -n "$n" "$work/back.img" "$f" && same_data=1
    if [ "$type" = "$hfe_type" ] && [ $same_list = 1 ] && [ $same_data = 1 ]; then
        ok=$((ok+1))
    else
        failed=$((failed+1))
        echo "MISMATCH $f: $type -> $hfe_type, files $same_list, sectors $same_data"
    fi
done < <(find "$@" -type f \( -iname '*.img' -o -iname '*.ima' -o -iname '*.st' -o -iname '*.kdi' \
             -o -iname '*.odi' -o -iname '*.fdd' -o -iname '*.bkd' -o -iname '*.rtd' -o -iname '*.dsk' \
             -o -iname '*.cpm' -o -iname '*.gmd' \) -print0)

echo "images $total, not opened as raw $skipped, passed $ok, failed $failed"
[ $failed = 0 ]
