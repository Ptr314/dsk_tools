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
  `FAMILY:VARIANT` (`TYPE_CPM:IRISHA-360-INT`, `TYPE_FAT:ST-720`). For CP/M and FAT the geometry
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

## Agat 840K and AIM

Enough of the recent work touches this that the layout is worth stating. A sector is

```
GAP ($AA) | DESYNC | 95 6A | volume track sector 5A | GAP | DESYNC | 6A 95 | 256 bytes | CRC 5A | GAP
```

with 21 sectors per track, 160 tracks, and the CRC an 8 bit sum with the carry added back.

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
