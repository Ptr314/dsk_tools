# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`dsk_tools` is a static C++11 library for reading, writing and converting floppy disk images of
retro computers (Agat, Apple II, a range of CP/M machines, PK8000, MS-DOS and Atari ST FAT
diskettes), plus two command line tools built on it. It has no Qt dependency but is written to be
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

There is no automated test suite. `test/` is git-ignored and holds sample images plus an older
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
(`src/dsk_tools.cpp`) does that for a decoded MFM track and `LoaderAIM::detect_sector_size()`
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
