# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`dsk_tools` is a static C++11 library for reading, writing and converting floppy disk images of
retro computers (Agat, Apple II, a range of CP/M machines, PK8000, MS-DOS and Atari ST FAT
diskettes, RT-11 disks of the DVK, BK and UKNC), plus two command line tools built on it. It has no Qt dependency but is written to be
consumed by one: **DISK Commander** (https://github.com/Ptr314/dsk_commander) uses this repo as a
submodule under `src/libs/dsk_tools`, so a public API change here ripples into the GUI.

User facing documentation is in Russian (`README.md`), code and comments are in English.

## Build

Development build (Ninja + any of MinGW / MSVC / GCC / Clang):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Targets: `dsk_tools` (the library), `fddconv` and `aim2hfe` (the tools, in `utils/`).
`-DENABLE_DSK_TOOLS=OFF` builds the library alone.

`dsk_tools` stands on `dsk_tools_core` (`include/dsk_tools/core.h`): definitions, `utils`,
`errors`, `host_helpers`, the track encodings of `src/disk_codecs.cpp` (4-and-4, GCR 6-and-2,
Agat MFM, whole Agat 140/840 track images) and `src/hfe.cpp`: the HFE container
(`hfe_read`/`hfe_write`, both sides interleaved in 512 byte blocks, cells LSB first) and the IBM
FM/MFM bit cell codecs (`fm_encode`/`fm_decode`, `mfm_encode`/`mfm_decode`, a flag per byte for
a sync byte or address mark without its clock; decoding re-takes the byte grid at every mark),
and `src/track_formats.cpp`: whole IBM MFM, IBM 3740 FM and DVK MX tracks (see "Whole tracks"
below). eCat3 links the core alone, so nothing in those files may reach a loader, an image, a
file system or a viewer; `dsk_tools.h` includes `core.h`, so the full library's API is unchanged.

Release archives are produced by the scripts in `.build/`: `build-win-mingw.bat` (x86_64),
`build-win-i386.bat`, `build-win-msvc.bat`, `build-linux.sh`, `build-macos.sh`. Each one
configures a Release build, builds both tools, copies them to `.build/release/<name>/` and zips
it. The Windows scripts get the toolchain from `.build/vars-*.cmd`, which hardcode paths under
`C:\DEV\Qt` — those need editing on another machine.

Two things that bite:

- Source files are listed explicitly in `CMakeLists.txt`; a new file that is not listed simply
  does not get compiled.
- `src/diskdefs` is embedded into `fddconv` at configure time through
  `utils/diskdefs_embed.h.in`, so editing it re-runs CMake (the GUI loads the same file from Qt
  resources instead).

## Testing

The whole track code has tests in `tools/`, built with `-DENABLE_DSK_TESTS=ON` and run by
`ctest`:

- `test_tracks` (core only) - IBM/MX tracks: digests of the tracks eCat3 writes (fail them and
  the emulator's `.hfe` files change), round trips, the index mark, damaged CRCs and IDs, write
  splices, bit phase, every geometry through `hfe_write`/`hfe_read`/`hfe_probe_format`. HFE files
  on its command line must read without bad sectors (eCat3's `tests/results/*.hfe`).
- `test_hfe_io <image> <type>` - a raw image through the public API: written to HFE, two sectors
  damaged, then detection, loading, bad sectors, filesystem, `load_structured()`, `file_info()`.
  `-DDSK_TEST_SAMPLES=<docs/samples>` registers it for a few sample images.
- `tools/hfe_roundtrip.sh <fddconv> <dirs...>` - raw -> HFE -> raw for every image found: same
  type, same file list, same sectors.

Everything else has no automated suite. `test/` is git-ignored and holds sample images plus an older
`fddconv.exe` kept for before/after comparison. Verification is done by running the tools over
sample images (the parent repo's `docs/samples`) and diffing the results, for example:

```bash
python tools/aim_extract.py disk.aim ref.dsk   # sectors straight from the AIM
aim2hfe disk.aim -o disk.hfe
fddconv disk.hfe -o test.dsk                   # sectors after a conversion round trip
cmp ref.dsk test.dsk
```

`tools/` holds Python 3 scripts for analysing AIM dumps (`aim_scan.py` reports everything that
does not fit the standard track layout); `tools/aim-anomalies.md` is the write-up of what they
found in the sample collection, including which known defects are still open.

## Architecture

The read path is a chain of factories, all of which live in `src/dsk_tools.cpp` and dispatch on
string ids:

```
file → detect_fdd_type()  → format_id ("FILE_HXC_HFE"), type_id ("TYPE_AGAT_840"), filesystem_id
     → prepare_image()    → create_loader() → Loader subclass → diskImage subclass
     → prepare_filesystem() → fileSystem subclass
     → fs->dir() / get_file() ...
```

Writing goes through `create_writer()`. Adding a format means a new `Loader`/`Writer` subclass, a
line in the corresponding factory and detection logic in `detect_fdd_type()`.

Key points that are not obvious from a single file:

- **Format and filesystem are independent dimensions.** `type_id` is two-level,
  `FAMILY:VARIANT` (`TYPE_CPM:IRISHA-360-INT`, `TYPE_FAT:ST-720`, `TYPE_OTHER:PRODOS-800`). For CP/M and FAT the geometry
  comes from `src/diskdefs` (cpmtools syntax, parsed by `parse_diskdefs()`), not from a C++
  class — prefer adding a `diskdefs` entry over a new `diskImage` subclass.
- **Filesystem capabilities are declared, not assumed.** `fileSystem::get_caps()` returns an
  `FSCaps` bitmask (read/write/mkdir/rename/…); the base class returns `NotImplementedYet` for
  every operation, so a filesystem implements only what it supports.
- **Viewers self-register** in the `ViewerManager` singleton. `register_all_viewers()` sits in its
  own translation unit (`src/viewers/register_viewers.cpp`) on purpose: a tool that never displays
  a file then links neither the viewers nor the BASIC detokenizers.
- **Errors are `Result{ErrorCode, message}`**, `operator bool()` for `if (res)`. Messages are
  wrapped in `QT_TRANSLATE_NOOP("errors", ...)` so the GUI can translate them — reuse an existing
  string where possible, since a new one needs a translation update in the GUI repo.
  `decode_error()` (`src/errors.cpp`) turns the code into text for the CLI.
- **Log and info strings use `{$TOKEN}` placeholders** (`{$TRACK}`, `{$SIZE}`, …) which the GUI
  substitutes (its `src/placeholders.h`). `file_info()` output is built this way and is only
  consumed by the GUI, not by the CLI tools.
- **Host file I/O goes through `host_helpers`** (`UTF8_ifstream` / `UTF8_ofstream`) so that
  Cyrillic paths work on Windows. On macOS `host_helpers.cpp` is compiled as Objective-C++.

## Conventions

- **C++11 only.** The i386 release is built with MinGW 4.9.2, so no C++14/17 in the library. The
  CLI tools are the exception: they switch to C++17 under MSVC because cxxopts needs it.
- Every file starts with the SPDX `GPL-3.0-or-later` header and a one line description.
- Release binaries are size-tuned (`-ffunction-sections -fdata-sections` + `--gc-sections -s`,
  `-static` on MinGW). Archive members are pulled in before `--gc-sections` runs and static
  initializers are GC roots, so **keep large tables and registration code in their own
  translation units** (`charmaps.cpp`, `errors.cpp`, `agat_charconv.cpp`, `register_viewers.cpp`).
  Folding them back into a shared file inflates both tools by hundreds of kilobytes.

## Onix OS

Onix (ONYX) is a port of Acorn MOS to the Agat. Its disks are ordinary 840 Kb Agat images
(`TYPE_AGAT_840`) with a filesystem of their own, and a block is one 256 byte sector numbered
straight through the disk (`block = track * 21 + sector`).

- Block 0 is the boot sector. **Blocks 1..20 are a 16 bit allocation table**, entry `i` at the
  absolute offset `0x100 + 2*i`, so it describes 2560 blocks. `entry[0]` is not a chain link but
  the first block of the root directory. `$FFFE` ends a chain, `$FFFF` is the area the OS image
  occupies (a single run starting at entry 1) and `0` is a free block.
- A directory is a chain of blocks holding **12 entries of 21 bytes** each (offsets 0..251, the
  last 4 bytes unused). A name starting with `$00` ends the entries *of that block*, the chain
  goes on; `$FF` marks a deleted entry.
- An entry is `name[10]`, month, day, start block, and three words whose meaning depends on
  `attributes & $C0`: `$40` a file (load, **length**, exec), `$80` a sequential `*SPOOL`/`OPENOUT`
  file (**length**, 0, 0), `$C0` a subdirectory. The OS branches on exactly those bits
  (`LDA attr / AND #$C0 / CMP #$80`) to pick where the length comes from.

**Known limitation.** The table covers 2560 blocks but an 840 Kb disk has 3360. `ONIX1_20.AIM`
has two files (`LOGO.LETTERS`, `LOGO.SECT`) starting past that, whose table entries would fall
inside the OS image. `fsOnix::block_chain()` stops at the edge of the table rather than reading
code as if it were a chain, so such a file reads back truncated to its first block. Whether a
larger volume keeps a second table somewhere is still open; no sample answers it.

Onix disks carry **two character sets at once**, so `fsOnix::get_charmap()` names the one
the viewer should start on (`onix`):

- **Documents** (the `WORD`/`TEXTS` folders, `!BOOT`, anything the OS wrote as text) are 8 bit
  **KOI-8**: Cyrillic in `$C0..$FF`, Latin left as plain ASCII. Measured over the 47 documents
  of the two samples: 61% of the bytes are in the Cyrillic block, 0.13% are `$80..$BF`
  formatting markers (`$80` prefixes a word processor command line), the rest is ASCII, and
  the letter frequency read that way comes out о е а и т н р с в п м л к д — Russian.

  On top of the glyphs they carry the layout codes of the word processor, which is why the
  `onix` charmap exists next to `koi8_r` rather than reusing it: **`$1A` is one space of
  justification padding** (21 703 of them across the samples — restoring them reproduces the
  original 65 column lines exactly), `$1C` and `$1D` bracket an emphasised heading, and `$0B`
  opens a non indented line. Above all `$1A` is *not* the end of text: `koi8_r` inherits the
  CP/M convention that it is, and reading a 20 Kb document with that charmap stops after the
  first screen.
- **BASIC sources** written under `*RUS` keep their Cyrillic as 7 bit **KOI-7**: the author
  types `"W monohromnoj grafike"` and the terminal shows «В монохромной графике». Those bytes
  are also perfectly good Latin, so nothing in the file distinguishes the two and the reader
  has to switch to КОИ-7 Н2 by hand. Onix itself only resolves it at display time.

Programs are tokenized **BBC BASIC** (`viewer_basic_bbc.cpp`): records of
`<CR><line hi><line lo><record length>` ending with `<CR><FF>`, tokens `$80..$FF` from the
standard Acorn BASIC II table (`BBC_tokens` in `bas_tokens.h`, verified against the keyword table
in the Onix system area). `$8D` is not a keyword but the marker of an encoded line number: the
three bytes after it carry the target with the top two bits of each half folded into the first
one and the result flipped with `$54`.


## Agat 840K/880K and AIM

Enough of the recent work touches this that the layout is worth stating. A sector is

```
GAP ($AA) | DESYNC | 95 6A | volume track sector 5A | GAP | DESYNC | 6A 95 | 256 bytes | CRC 5A | GAP
```

with 21 sectors per track, 160 tracks, and the CRC an 8 bit sum with the carry added back.

**The sector size is not a constant.** Nippel OS disks (`TYPE_AGAT_880`, 880 Kb) use the very
same layout with **11 sectors of 512 bytes** per track and hold a ProDOS volume. Nothing on
the disk states which of the two it is, so it is deduced from the data: the checksum after a
data field only adds up for the length the disk was formatted with. `detect_agat_sector_size()`
(`src/disk_codecs.cpp`) does that for a decoded MFM track and `LoaderAIM::detect_sector_size()`
for an AIM dump; `decode_agat_840_track()` takes the geometry as parameters. An explicitly
requested `type_id` always wins over the detection.

An AIM dump is 160 × 6464 cells of (data byte, AIM command). The commands are `$01`/`$80`/`$81`
DESYNC, `$02` end of track, `$03`/`$13` index pulse; anything else is payload. A dump covers
slightly more than one revolution, so **a track is a ring**: its last field wraps to the
beginning and the first cells may be a re-read of the last ones. Both `LoaderAIM` and
`AIM2HFEConverter` rely on that, and sectors are placed by the number from the address field —
a dump does not have to start at sector 0.

`aim2hfe` re-encodes a whole track (gaps, DESYNC marks and copy protection included) instead of
going through sectors. Its algorithm follows Oleksandr Kapitanenko's `agath-aim-to-hfe.pl` and is
byte-identical to it on every image that script can convert. Two deliberate extensions are
documented in `README.md` and `tools/aim-anomalies.md`: gap cells left unread as $00 are counted
as gap and restored to $AA, and the `$81` DESYNC variant is recognised.

## Whole tracks (HFE)

An HFE keeps the bit cells of every track, so a sector image has to be laid out on a track the
way a controller formats it, and read back by finding the fields again. Apart from the Agat
(own encoder in `writer_mfm.cpp`, own decoder in `disk_codecs.cpp`), this is driven by the
**`layout` of a diskdef** (`ibm-mfm`, `ibm-fm`, `dvk-mx`), which `prepare_image()` puts into
`DiskFormatParams::layout` together with `bitrate`/`rpm` and the gaps (`gap4a`, `gap1`, `gap2`,
`gap3`, `indexmark`; IBM values fitted to the turn when not given). Adding HFE support for a
CP/M or FAT machine is therefore a diskdefs line plus `config.json`, not code.

- `ibm_track_fields()` is the one parser of IBM tracks: it finds marks by the sync bytes
  (`A1 A1 A1` + FE/FB/F8, `C2 C2 C2 FC` for the index mark in MFM; a zero before the mark in FM,
  or the `special` flags of the cell decoder), checks **both CRCs**, and skips whatever lies
  between fields — the index mark, write splices (`DA 6E` after a sector in eCat3 output), the
  unformatted end of a track. A data field is taken only after an ID whose CRC passed.
- `track_from_cells()` places sectors by the number in their ID (from `sector_base`) and
  reports a `TrackStatus` (missing, bad CRC, deleted); `hfe_to_flat()` turns those into
  `m_bad_sectors`. `LoaderHXC_HFE::load_structured()` gives the explorer the physical order of
  the sectors with their C/H/R, and `file_info()` lists the fields of every track.
- `detect_fdd_type()` runs `hfe_probe_format()` (layout and geometry from the first tracks:
  two readable IDs are required) and then looks at the volume: RT-11 home block, FAT boot sector
  (PC when it has `55 AA` or a text OEM name, else ST), the CP/M directory behind 1/2/4 system
  cylinders for Korvet/Orion/Vector. Only when no IBM/MX layout is found does the Agat probe run.
- The RT-11 disks keep the exact tracks of the original eCat3 code (MY/MZ: no index mark, gaps
  42/22/36 via diskdefs; DX: 40/26/11/27), and eCat3 calls `ibm_mfm_format_track`,
  `ibm_*_read_track`, `ibm_*_find_marks`, `ibm_fm_find_sector`, `ibm_fm_put_sector` and the
  `dvk_mx_*` functions directly — **keep their signatures**; they are thin wrappers over the
  generic code now.

## RT-11

DEC RT-11 disks of the DVK, BK-0010/0011 and UKNC (`fs_rt11.cpp`, types `TYPE_RT11:*`). The
volume is a flat run of 512 byte blocks: block 1 is the home block (directory start at `0724`,
volume ID, owner, `DECRT11A`), the directory is a chain of two block segments from block 6, and
every file is one contiguous run whose start is the segment's first data block plus the lengths
of the entries before it. Entries are 7 words (status, 3 RAD50 words of name, length, job/channel,
date) plus the volume's extra bytes; the date keeps two "age" bits on top for years past 2003.

- **DX images are in physical sector order** (SIMH, 77 x 26 x 128). RT-11 skips track 0 and
  interleaves the rest itself: 2:1 within a track and a skew of 6 from track to track, so a
  `skewtab` cannot express it and `fsRT11::dx_sector()` maps a logical sector instead. 256256
  bytes is also the Irisha GMD-7012, so detection reads the directory through that mapping
  before picking `TYPE_RT11:DX` over CP/M.
- MX, MY and MZ images are linear. MY (DVK) and MZ (UKNC, BK) 800 Kb disks are byte for byte the
  same geometry; detection says MY, `.bkd` says MZ.
- `.rtd` images may carry a 256 byte header and be any length; `LoaderRAW` skips the header
  for RT-11 types and pads short files. A directory may describe a larger volume than the
  image holds (a hard disk partition cut down to a floppy), so free space and allocation are
  clamped to the blocks the image has.
- Writing works on a copy of the directory chain and writes it back only once the file has its
  place. Delete only flips the status to E.MPTY and keeps the name, which is what makes restore
  possible; empty areas are joined just before an allocation. A full segment is split into the
  next unused one (`highest + 1`).
