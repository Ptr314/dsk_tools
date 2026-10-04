// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the DEC RT-11 filesystem (BK, DVK, UKNC)

#include <cstring>
#include <cstdio>
#include <set>
#include <algorithm>
#include <ctime>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_rt11.h"

namespace dsk_tools {

    namespace {

        const char RAD50_CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";

        uint16_t get_word(const uint8_t * p)
        {
            return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
        }

        void put_word(uint8_t * p, uint16_t w)
        {
            p[0] = static_cast<uint8_t>(w & 0xFF);
            p[1] = static_cast<uint8_t>(w >> 8);
        }

        std::string octal(unsigned value)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%06o", value);
            return std::string(buffer);
        }

        bool is_live(const RT11_Entry & e)
        {
            return (e.status & (RT11_E_PERM | RT11_E_TENT)) != 0;
        }

        bool is_empty(const RT11_Entry & e)
        {
            return (e.status & RT11_E_MPTY) != 0;
        }

        // An empty area left by a deleted file keeps its name; one RT-11 made itself has none
        bool has_name(const RT11_Entry & e)
        {
            return e.name[0] != 0 || e.name[1] != 0 || e.name[2] != 0;
        }

        BYTES entry_bytes(const RT11_Entry & e)
        {
            BYTES out(RT11_ENTRY_LENGTH + e.extra.size(), 0);
            put_word(&out[0],  e.status);
            put_word(&out[2],  e.name[0]);
            put_word(&out[4],  e.name[1]);
            put_word(&out[6],  e.name[2]);
            put_word(&out[8],  e.length);
            put_word(&out[10], e.job);
            put_word(&out[12], e.date);
            std::copy(e.extra.begin(), e.extra.end(), out.begin() + RT11_ENTRY_LENGTH);
            return out;
        }

        bool entry_from_bytes(const BYTES & raw, RT11_Entry & e)
        {
            if (raw.size() < RT11_ENTRY_LENGTH) return false;
            e.status  = get_word(&raw[0]);
            e.name[0] = get_word(&raw[2]);
            e.name[1] = get_word(&raw[4]);
            e.name[2] = get_word(&raw[6]);
            e.length  = get_word(&raw[8]);
            e.job     = get_word(&raw[10]);
            e.date    = get_word(&raw[12]);
            e.extra.assign(raw.begin() + RT11_ENTRY_LENGTH, raw.end());
            return true;
        }

        // A segment ends with the E.EOS marker, which needs a word of its own
        bool parse_segment(const BYTES & raw, RT11_Segment & seg)
        {
            if (raw.size() < RT11_SEGMENT_SIZE) return false;
            seg.total   = get_word(&raw[0]);
            seg.next    = get_word(&raw[2]);
            seg.highest = get_word(&raw[4]);
            seg.extra   = get_word(&raw[6]);
            seg.start   = get_word(&raw[8]);
            seg.entries.clear();

            if ((seg.extra & 1) != 0) return false;
            const unsigned size = RT11_ENTRY_LENGTH + seg.extra;
            if (RT11_SEGMENT_HEADER + size + 2 > RT11_SEGMENT_SIZE) return false;

            unsigned offset = RT11_SEGMENT_HEADER;
            unsigned block = seg.start;
            while (offset + 2 <= RT11_SEGMENT_SIZE) {
                const uint16_t status = get_word(&raw[offset]);
                if (status & RT11_E_EOS) break;
                // Anything else would be garbage past a lost end marker
                if ((status & (RT11_E_TENT | RT11_E_MPTY | RT11_E_PERM)) == 0) break;
                if (offset + size > RT11_SEGMENT_SIZE) break;

                RT11_Entry e;
                entry_from_bytes(BYTES(raw.begin() + offset, raw.begin() + offset + size), e);
                e.start = block;
                block += e.length;
                seg.entries.push_back(e);
                offset += size;
            }
            return true;
        }

        const std::set<std::string> & text_types()
        {
            static const std::set<std::string> txts = {
                ".txt", ".mac", ".for", ".bas", ".com", ".ctl", ".doc", ".lst", ".map",
                ".pas", ".c", ".h", ".hlp", ".ans", ".dat", ".ini", ".me"
            };
            return txts;
        }

    }

    fsRT11::fsRT11(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsRT11::get_caps()
    {
        return    FSCaps::Protect | FSCaps::Date   | FSCaps::Export
                | FSCaps::Delete  | FSCaps::Add    | FSCaps::Rename
                | FSCaps::Metadata | FSCaps::Restore;
    }

    // Text files of the DEC machines are 7 bit KOI-7, Latin and Cyrillic switched by SO/SI
    std::string fsRT11::get_charmap() const
    {
        return "koi7_n0_n1";
    }

    std::vector<std::string> fsRT11::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    std::vector<std::string> fsRT11::get_add_file_formats()
    {
        return {"FILE_BINARY"};
    }

    // ---------------------------------------------------------------- RAD50, names, dates

    std::string fsRT11::rad50_decode(uint16_t word)
    {
        if (word >= 40u * 40u * 40u) return "???";
        std::string s(3, ' ');
        s[0] = RAD50_CHARS[word / 1600];
        s[1] = RAD50_CHARS[(word / 40) % 40];
        s[2] = RAD50_CHARS[word % 40];
        return s;
    }

    bool fsRT11::rad50_encode(const std::string & text, uint16_t & word)
    {
        word = 0;
        for (size_t i = 0; i < 3; i++) {
            const char c = (i < text.size()) ? text[i] : ' ';
            const char * p = std::strchr(RAD50_CHARS, c);
            if (c == '\0' || p == nullptr) return false;
            word = static_cast<uint16_t>(word * 40 + (p - RAD50_CHARS));
        }
        return true;
    }

    std::string fsRT11::entry_name(const RT11_Entry & entry)
    {
        const std::string base = trim(rad50_decode(entry.name[0]) + rad50_decode(entry.name[1]), " ");
        const std::string ext  = trim(rad50_decode(entry.name[2]), " ");
        return ext.empty() ? base : (base + "." + ext);
    }

    Result fsRT11::make_name(const std::string & file_name, uint16_t out[3])
    {
        const std::string name = to_upper(file_name);
        std::string base, ext;
        const auto dot = name.find_last_of('.');
        if (dot == std::string::npos) {
            base = name;
        } else {
            base = name.substr(0, dot);
            ext  = name.substr(dot + 1);
        }
        if (base.empty() || base.size() > 6 || ext.size() > 3)
            return Result::error(ErrorCode::InvalidName);

        // RAD50 holds '.' and '%' as well, but RT-11 does not take them in a name
        for (const std::string * part : {&base, &ext})
            for (const char c : *part)
                if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '$'))
                    return Result::error(ErrorCode::InvalidName);

        base.resize(6, ' ');
        ext.resize(3, ' ');
        if (!rad50_encode(base.substr(0, 3), out[0])
         || !rad50_encode(base.substr(3, 3), out[1])
         || !rad50_encode(ext, out[2]))
            return Result::error(ErrorCode::InvalidName);
        return Result::ok();
    }

    Result fsRT11::make_name(const UniversalFile & uf, uint16_t out[3])
    {
        // A file coming from another RT-11 disk keeps its name word for word
        if (uf.fs == FS::RT11 && uf.original_name.size() == 6) {
            for (unsigned i = 0; i < 3; i++) out[i] = get_word(&uf.original_name[i * 2]);
            return Result::ok();
        }
        return make_name(get_filename(uf.name), out);
    }

    // The year is kept as an offset from 1972 in 5 bits, and since V5.5 two more
    // "age" bits on top of the word extend it to 2099
    FileDate fsRT11::decode_date(uint16_t date)
    {
        FileDate result;
        if (date == 0) return result;
        const unsigned month = (date >> 10) & 017;
        const unsigned day   = (date >> 5) & 037;
        if (month == 0 || month > 12 || day == 0) return result;

        result.year  = static_cast<uint16_t>(1972 + (date & 037) + ((date >> 14) & 3) * 32);
        result.month = static_cast<uint8_t>(month);
        result.day   = static_cast<uint8_t>(day);
        return result;
    }

    std::string fsRT11::date_to_string(uint16_t date)
    {
        const FileDate d = decode_date(date);
        if (!d.valid()) return "";

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02u.%02u.%04u", d.day, d.month, d.year);
        return std::string(buffer);
    }

    bool fsRT11::string_to_date(const std::string & s, uint16_t & date)
    {
        const std::string text = trim(s);
        if (text.empty()) {
            date = 0;
            return true;
        }
        unsigned day = 0, month = 0, year = 0;
        if (std::sscanf(text.c_str(), "%u.%u.%u", &day, &month, &year) != 3) return false;
        if (year < 100) year += (year < 72) ? 2000 : 1900;
        if (year < 1972 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) return false;

        const unsigned offset = year - 1972;
        date = static_cast<uint16_t>((month << 10) | (day << 5) | (offset & 037) | ((offset >> 5) << 14));
        return true;
    }

    uint16_t fsRT11::today()
    {
        const std::time_t t = std::time(nullptr);
        const std::tm * lt = std::localtime(&t);
        if (!lt) return 0;
        const unsigned year = static_cast<unsigned>(lt->tm_year) + 1900;
        if (year < 1972 || year > 2099) return 0;

        const unsigned offset = year - 1972;
        return static_cast<uint16_t>(((lt->tm_mon + 1) << 10) | (lt->tm_mday << 5) | (offset & 037) | ((offset >> 5) << 14));
    }

    std::string fsRT11::home_text(const BYTES & home, unsigned offset)
    {
        std::string s;
        for (unsigned i = 0; i < RT11_HB_TEXT_LENGTH && offset + i < home.size(); i++) {
            const uint8_t c = home[offset + i];
            s += (c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : ' ';
        }
        return trim(s, " ");
    }

    // ---------------------------------------------------------------- Blocks

    // The part of an area that the image really holds: a directory written for a larger
    // volume (a hard disk partition cut down to a floppy image) runs past its end
    unsigned fsRT11::blocks_inside(const RT11_Entry & entry) const
    {
        if (entry.start >= total_blocks) return 0;
        return std::min<unsigned>(entry.length, total_blocks - entry.start);
    }

    bool fsRT11::is_dx() const
    {
        return image->get_sectors() == RT11_DX_SECTORS
            && image->get_sector_size() == RT11_DX_SECTOR_SIZE
            && image->get_tracks() == RT11_DX_TRACKS;
    }

    // RT-11's DX driver: 2:1 interleave inside a track, a skew of 6 sectors from one
    // track to the next, and track 0 left unused
    void fsRT11::dx_sector(unsigned logical, unsigned & track, unsigned & sector)
    {
        const unsigned t = logical / RT11_DX_SECTORS;
        const unsigned s = logical % RT11_DX_SECTORS;
        sector = (s * 2 + (s >= RT11_DX_SECTORS / 2 ? 1 : 0) + 6 * t) % RT11_DX_SECTORS;
        track = t + 1;
    }

    // The sectors {head, track, sector} that make up a block, in order
    bool fsRT11::block_sectors(unsigned block, std::vector<std::array<unsigned, 3>> & sectors) const
    {
        sectors.clear();
        if (block >= total_blocks) return false;

        if (is_dx()) {
            const unsigned per_block = RT11_BLOCK_SIZE / RT11_DX_SECTOR_SIZE;
            for (unsigned i = 0; i < per_block; i++) {
                unsigned track, sector;
                dx_sector(block * per_block + i, track, sector);
                sectors.push_back(std::array<unsigned, 3>{{0, track, sector}});
            }
            return true;
        }

        // Everything else is a plain run of sectors, both sides of a cylinder in turn
        const unsigned ss    = image->get_sector_size();
        const unsigned spt   = image->get_sectors();
        const unsigned heads = image->get_heads();
        if (ss == 0 || spt == 0 || heads == 0 || RT11_BLOCK_SIZE % ss != 0) return false;

        const unsigned per_block = RT11_BLOCK_SIZE / ss;
        for (unsigned i = 0; i < per_block; i++) {
            const unsigned lsn = block * per_block + i;
            const unsigned head  = (lsn / spt) % heads;
            const unsigned track = (lsn / spt) / heads;
            if (track >= image->get_tracks()) return false;
            sectors.push_back(std::array<unsigned, 3>{{head, track, lsn % spt}});
        }
        return true;
    }

    Result fsRT11::read_block(unsigned block, BYTES & out) const
    {
        std::vector<std::array<unsigned, 3>> sectors;
        if (!block_sectors(block, sectors)) return Result::error(ErrorCode::ReadError);

        const unsigned ss = RT11_BLOCK_SIZE / static_cast<unsigned>(sectors.size());
        out.assign(RT11_BLOCK_SIZE, 0);
        for (size_t i = 0; i < sectors.size(); i++) {
            const uint8_t * p = image->get_sector_data(sectors[i][0], sectors[i][1], sectors[i][2]);
            if (!p) return Result::error(ErrorCode::ReadError);
            std::memcpy(&out[i * ss], p, ss);
        }
        return Result::ok();
    }

    // Writes `size` bytes of `data` into the block, the rest is zero filled
    Result fsRT11::write_block(unsigned block, const uint8_t * data, size_t size)
    {
        std::vector<std::array<unsigned, 3>> sectors;
        if (!block_sectors(block, sectors)) return Result::error(ErrorCode::WriteError);

        const unsigned ss = RT11_BLOCK_SIZE / static_cast<unsigned>(sectors.size());
        for (size_t i = 0; i < sectors.size(); i++) {
            uint8_t * p = image->get_sector_data(sectors[i][0], sectors[i][1], sectors[i][2]);
            if (!p) return Result::error(ErrorCode::WriteError);
            const size_t offset = i * ss;
            const size_t to_copy = (data != nullptr && offset < size) ? std::min<size_t>(ss, size - offset) : 0;
            if (to_copy) std::memcpy(p, data + offset, to_copy);
            if (to_copy < ss) std::memset(p + to_copy, 0, ss - to_copy);
        }
        return Result::ok();
    }

    // ---------------------------------------------------------------- Directory

    unsigned fsRT11::directory_block(const BYTES & home, unsigned total_blocks)
    {
        if (home.size() < RT11_BLOCK_SIZE) return RT11_DEFAULT_DIR;
        const unsigned block = get_word(&home[RT11_HB_DIR_BLOCK]);
        // Some volumes leave the field empty; RT-11 itself always starts at block 6
        if (block < 2 || block + RT11_SEGMENT_BLOCKS > total_blocks) return RT11_DEFAULT_DIR;
        return block;
    }

    bool fsRT11::segment_is_valid(const BYTES & segment, unsigned dir_block, unsigned total_blocks)
    {
        if (segment.size() < RT11_SEGMENT_HEADER + 2) return false;
        const unsigned total   = get_word(&segment[0]);
        const unsigned next    = get_word(&segment[2]);
        const unsigned highest = get_word(&segment[4]);
        const unsigned extra   = get_word(&segment[6]);
        const unsigned start   = get_word(&segment[8]);

        if (total == 0 || total > RT11_MAX_SEGMENTS) return false;
        if (next > total || highest == 0 || highest > total) return false;
        if ((extra & 1) != 0 || RT11_SEGMENT_HEADER + RT11_ENTRY_LENGTH + extra + 2 > RT11_SEGMENT_SIZE) return false;
        if (start != dir_block + total * RT11_SEGMENT_BLOCKS || start >= total_blocks) return false;

        const uint16_t status = get_word(&segment[RT11_SEGMENT_HEADER]);
        return (status & (RT11_E_TENT | RT11_E_MPTY | RT11_E_PERM | RT11_E_EOS)) != 0;
    }

    Result fsRT11::load_segment(unsigned number, RT11_Segment & segment) const
    {
        BYTES raw;
        for (unsigned i = 0; i < RT11_SEGMENT_BLOCKS; i++) {
            BYTES block;
            auto res = read_block(dir_block + (number - 1) * RT11_SEGMENT_BLOCKS + i, block);
            if (!res) return res;
            raw.insert(raw.end(), block.begin(), block.end());
        }
        if (!parse_segment(raw, segment)) return Result::error(ErrorCode::DirError);
        segment.number = number;
        return Result::ok();
    }

    Result fsRT11::save_segment(const RT11_Segment & segment)
    {
        const unsigned first = dir_block + (segment.number - 1) * RT11_SEGMENT_BLOCKS;

        // Whatever lies past the end marker stays as it was
        BYTES raw;
        for (unsigned i = 0; i < RT11_SEGMENT_BLOCKS; i++) {
            BYTES block;
            auto res = read_block(first + i, block);
            if (!res) return res;
            raw.insert(raw.end(), block.begin(), block.end());
        }

        put_word(&raw[0], segment.total);
        put_word(&raw[2], segment.next);
        put_word(&raw[4], segment.highest);
        put_word(&raw[6], segment.extra);
        put_word(&raw[8], segment.start);

        unsigned offset = RT11_SEGMENT_HEADER;
        for (const RT11_Entry & e : segment.entries) {
            RT11_Entry copy = e;
            copy.extra.resize(segment.extra, 0);
            const BYTES bytes = entry_bytes(copy);
            if (offset + bytes.size() + 2 > RT11_SEGMENT_SIZE) return Result::error(ErrorCode::DirErrorAllocateDirEntry);
            std::copy(bytes.begin(), bytes.end(), raw.begin() + offset);
            offset += static_cast<unsigned>(bytes.size());
        }
        put_word(&raw[offset], RT11_E_EOS);

        for (unsigned i = 0; i < RT11_SEGMENT_BLOCKS; i++) {
            auto res = write_block(first + i, &raw[i * RT11_BLOCK_SIZE], RT11_BLOCK_SIZE);
            if (!res) return res;
        }
        return Result::ok();
    }

    Result fsRT11::load_directory(std::vector<RT11_Segment> & chain) const
    {
        chain.clear();
        unsigned number = 1;
        unsigned total = RT11_MAX_SEGMENTS;
        while (number != 0) {
            if (number > total || chain.size() >= total) return Result::error(ErrorCode::DirError);
            RT11_Segment seg;
            auto res = load_segment(number, seg);
            if (!res) return res;
            if (chain.empty()) total = seg.total;
            number = seg.next;
            chain.push_back(seg);
        }
        return Result::ok();
    }

    // One more entry has to fit together with the end of segment marker
    bool fsRT11::segment_has_room(const RT11_Segment & segment) const
    {
        const unsigned size = RT11_ENTRY_LENGTH + segment.extra;
        return RT11_SEGMENT_HEADER + (segment.entries.size() + 1) * size + 2 <= RT11_SEGMENT_SIZE;
    }

    // Moves the second half of a full segment to the next unused one
    Result fsRT11::split_segment(std::vector<RT11_Segment> & chain, size_t index)
    {
        RT11_Segment & seg = chain[index];
        const size_t n = seg.entries.size();
        const unsigned number = chain[0].highest + 1u;
        if (n < 2 || number > chain[0].total) return Result::error(ErrorCode::DirErrorAllocateDirEntry);

        const size_t half = n / 2;
        RT11_Segment added;
        added.number  = number;
        added.total   = seg.total;
        added.next    = seg.next;
        added.extra   = seg.extra;
        added.start   = static_cast<uint16_t>(seg.entries[half].start);
        added.entries.assign(seg.entries.begin() + half, seg.entries.end());

        seg.entries.resize(half);
        seg.next = static_cast<uint16_t>(number);

        chain[0].highest = static_cast<uint16_t>(number);
        added.highest = chain[0].highest;
        chain.insert(chain.begin() + index + 1, added);
        return Result::ok();
    }

    // Joins the empty areas that lie next to each other and drops the empty ones
    // of no length. A joined area no longer stands for one deleted file, so it loses its name.
    void fsRT11::consolidate(RT11_Segment & segment)
    {
        auto & v = segment.entries;
        for (size_t i = 0; i + 1 < v.size(); ) {
            if (is_empty(v[i]) && is_empty(v[i + 1])) {
                v[i].length = static_cast<uint16_t>(v[i].length + v[i + 1].length);
                v[i].status = RT11_E_MPTY;
                v[i].name[0] = v[i].name[1] = v[i].name[2] = 0;
                v[i].date = 0;
                v.erase(v.begin() + i + 1);
            } else {
                i++;
            }
        }
        for (size_t i = 0; i < v.size() && v.size() > 1; ) {
            if (is_empty(v[i]) && v[i].length == 0) v.erase(v.begin() + i);
            else i++;
        }
        unsigned block = segment.start;
        for (auto & e : v) {
            e.start = block;
            block += e.length;
        }
    }

    bool fsRT11::locate(const std::vector<RT11_Segment> & chain, const UniversalFile & uf, size_t & seg_index, size_t & entry_index) const
    {
        if (uf.position.size() < 3 || uf.original_name.size() != 6) return false;
        for (size_t s = 0; s < chain.size(); s++) {
            if (chain[s].number != uf.position[0]) continue;
            if (uf.position[1] >= chain[s].entries.size()) return false;
            const RT11_Entry & e = chain[s].entries[uf.position[1]];
            for (unsigned i = 0; i < 3; i++)
                if (e.name[i] != get_word(&uf.original_name[i * 2])) return false;
            if (e.start != uf.position[2]) return false;
            if (uf.is_deleted ? !is_empty(e) : !is_live(e)) return false;
            seg_index = s;
            entry_index = uf.position[1];
            return true;
        }
        return false;
    }

    bool fsRT11::name_exists(const std::vector<RT11_Segment> & chain, const uint16_t name[3], const RT11_Entry * except) const
    {
        for (const auto & seg : chain)
            for (const auto & e : seg.entries)
                if (&e != except && is_live(e)
                    && e.name[0] == name[0] && e.name[1] == name[1] && e.name[2] == name[2])
                    return true;
        return false;
    }

    // ---------------------------------------------------------------- Open, listing, reading

    Result fsRT11::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        if (is_dx()) {
            total_blocks = RT11_DX_BLOCKS;
        } else {
            const unsigned bytes = image->get_heads() * image->get_tracks() * image->get_sectors() * image->get_sector_size();
            total_blocks = bytes / RT11_BLOCK_SIZE;
        }
        if (total_blocks <= RT11_DEFAULT_DIR + RT11_SEGMENT_BLOCKS)
            return Result::error(ErrorCode::OpenBadFormat);

        auto res = read_block(RT11_HOME_BLOCK, home);
        if (!res) return res;
        dir_block = directory_block(home, total_blocks);

        BYTES first;
        res = read_block(dir_block, first);
        if (!res) return res;
        if (!segment_is_valid(first, dir_block, total_blocks))
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "RT-11: invalid directory"));

        is_open = true;
        std::vector<RT11_Segment> chain;
        res = load_directory(chain);
        if (!res) {
            is_open = false;
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "RT-11: invalid directory"));
        }
        volume_id = -1;
        return Result::ok();
    }

    UniversalFile fsRT11::make_universal_file(const RT11_Entry & entry, unsigned segment, unsigned index) const
    {
        UniversalFile f{};
        f.fs           = get_fs();
        f.name         = entry_name(entry);
        f.size         = static_cast<uint32_t>(entry.length) * RT11_BLOCK_SIZE;
        f.is_dir       = false;
        f.is_deleted   = is_empty(entry);
        f.is_protected = (entry.status & RT11_E_PROT) != 0;
        f.attributes   = entry.status;
        f.date         = decode_date(entry.date);

        f.type_preferred = PreferredType::Binary;
        if (text_types().count(to_lower(get_file_ext(f.name))) != 0) f.type_preferred = PreferredType::Text;

        f.original_name.resize(6);
        for (unsigned i = 0; i < 3; i++) put_word(&f.original_name[i * 2], entry.name[i]);

        f.metadata = entry_bytes(entry);

        // Where the entry lives, so that the write operations can find it again
        f.position = {segment, index, entry.start};
        return f;
    }

    Result fsRT11::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        files.clear();

        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        for (const auto & seg : chain) {
            for (size_t i = 0; i < seg.entries.size(); i++) {
                const RT11_Entry & e = seg.entries[i];
                if (is_live(e) || (show_deleted && is_empty(e) && has_name(e) && e.length > 0))
                    files.push_back(make_universal_file(e, seg.number, static_cast<unsigned>(i)));
            }
        }
        return Result::ok();
    }

    Result fsRT11::find_file(const std::string & file_name, UniversalFile & fd)
    {
        Files files;
        auto res = dir(files, false);
        if (!res) return res;

        for (const UniversalFile & f : files) {
            if (f.name == to_upper(file_name)) {
                fd = f;
                return Result::ok();
            }
        }
        return Result::error(ErrorCode::NotFound);
    }

    Result fsRT11::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        RT11_Entry e;
        if (uf.position.size() < 3 || !entry_from_bytes(uf.metadata, e))
            return Result::error(ErrorCode::FileIncorrectFS);

        data.clear();
        data.reserve(static_cast<size_t>(e.length) * RT11_BLOCK_SIZE);
        for (unsigned i = 0; i < e.length; i++) {
            BYTES block;
            auto res = read_block(uf.position[2] + i, block);
            if (!res) return res;
            data.insert(data.end(), block.begin(), block.end());
        }
        return Result::ok();
    }

    std::string fsRT11::file_info(const UniversalFile & fd)
    {
        RT11_Entry e;
        if (fd.position.size() < 3 || !entry_from_bytes(fd.metadata, e)) return "";

        std::string flags;
        if (e.status & RT11_E_PERM) flags += " PERM";
        if (e.status & RT11_E_TENT) flags += " TENT";
        if (e.status & RT11_E_MPTY) flags += " MPTY";
        if (e.status & RT11_E_PROT) flags += " PROT";
        if (e.status & RT11_E_READ) flags += " READ";
        if (e.status & RT11_E_PRE)  flags += " PRE";

        const std::string date = date_to_string(e.date);

        std::string result;
        result += "{$DIRECTORY_ENTRY}:\n";
        result += "    {$FILE_NAME}: " + fd.name + " (" + octal(e.name[0]) + " " + octal(e.name[1]) + " " + octal(e.name[2]) + ")\n";
        result += "    {$RT11_STATUS}: " + octal(e.status) + flags + "\n";
        result += "    {$RT11_DATE}: " + (date.empty() ? std::string("<{$NO_DATE}>") : date) + " (" + octal(e.date) + ")\n";
        result += "    {$RT11_FIRST_BLOCK}: " + std::to_string(fd.position[2]) + "\n";
        result += "    {$RT11_LENGTH}: " + std::to_string(e.length) + "\n";
        result += "    {$SIZE}: " + std::to_string(static_cast<unsigned>(e.length) * RT11_BLOCK_SIZE) + " {$BYTES}\n";
        if (e.status & RT11_E_TENT)
            result += "    {$RT11_JOB}: " + octal(e.job) + "\n";
        result += "    {$RT11_SEGMENT}: " + std::to_string(fd.position[0]) + ", {$RT11_ENTRY}: " + std::to_string(fd.position[1]) + "\n";
        if (!e.extra.empty())
            result += "    {$RT11_EXTRA_BYTES}: " + toHexList(e.extra, "$") + "\n";
        return result;
    }

    std::string fsRT11::information()
    {
        if (!is_open) return "";

        std::vector<RT11_Segment> chain;
        if (!load_directory(chain) || chain.empty()) return "";

        unsigned used = 0, free = 0, files = 0, volume_end = 0;
        for (const auto & seg : chain)
            for (const auto & e : seg.entries) {
                if (is_live(e)) { used += e.length; files++; }
                else if (is_empty(e)) free += blocks_inside(e);
                volume_end = std::max(volume_end, e.start + e.length);
            }

        const RT11_Segment & first = chain[0];

        std::string result;
        result += "{$RT11_HOME_BLOCK}:\n";
        result += "    {$RT11_VOLUME_ID}: " + home_text(home, RT11_HB_VOLUME_ID) + "\n";
        result += "    {$RT11_OWNER}: " + home_text(home, RT11_HB_OWNER) + "\n";
        result += "    {$RT11_SYSTEM_ID}: " + home_text(home, RT11_HB_SYSTEM_ID) + "\n";
        result += "    {$RT11_VERSION}: " + trim(rad50_decode(get_word(&home[RT11_HB_VERSION])), " ") + "\n";
        result += "    {$RT11_CLUSTER_SIZE}: " + std::to_string(get_word(&home[RT11_HB_CLUSTER])) + "\n";
        result += "\n";

        result += "{$RT11_DIRECTORY}:\n";
        result += "    {$RT11_DIR_BLOCK}: " + std::to_string(dir_block) + "\n";
        result += "    {$RT11_SEGMENTS}: " + std::to_string(first.total) + "\n";
        result += "    {$RT11_SEGMENTS_USED}: " + std::to_string(chain.size()) + "\n";
        result += "    {$RT11_EXTRA_BYTES}: " + std::to_string(first.extra) + "\n";
        result += "    {$RT11_DATA_START}: " + std::to_string(first.start) + "\n";
        result += "    {$RT11_FILES}: " + std::to_string(files) + "\n";
        result += "\n";

        result += "{$RT11_BLOCKS}:\n";
        result += "    {$RT11_DISK_BLOCKS}: " + std::to_string(total_blocks) + "\n";
        // The directory may describe a larger volume than the image holds
        if (volume_end != total_blocks)
            result += "    {$RT11_VOLUME_BLOCKS}: " + std::to_string(volume_end) + "\n";
        result += "    {$RT11_USED_BLOCKS}: " + std::to_string(used) + "\n";
        result += "    {$RT11_FREE_BLOCKS}: " + std::to_string(free) + "\n";
        result += "    {$FREE_BYTES}: " + std::to_string(static_cast<uint64_t>(free) * RT11_BLOCK_SIZE) + "\n";
        return result;
    }

    void fsRT11::update_stats()
    {
        m_stats.int_values.clear();
        m_stats.int_values["image_size"] = image->get_heads() * image->get_tracks()
                                         * image->get_sectors() * image->get_sector_size();

        unsigned used = 0, free = 0;
        std::vector<RT11_Segment> chain;
        if (is_open && load_directory(chain)) {
            for (const auto & seg : chain)
                for (const auto & e : seg.entries) {
                    if (is_live(e)) used += e.length;
                    else if (is_empty(e)) free += blocks_inside(e);
                }
        }
        m_stats.int_values["total_space"]    = (used + free) * RT11_BLOCK_SIZE;
        m_stats.int_values["occupied_space"] = used * RT11_BLOCK_SIZE;
        m_stats.int_values["free_space"]     = free * RT11_BLOCK_SIZE;
        stats_valid = true;
    }

    SectorTypeMap fsRT11::get_sector_type_map()
    {
        SectorTypeMap result;
        if (!is_open) return result;

        std::vector<RT11_Segment> chain;
        if (!load_directory(chain) || chain.empty()) return result;

        auto mark = [&](unsigned block, SectorType type) {
            std::vector<std::array<unsigned, 3>> sectors;
            if (!block_sectors(block, sectors)) return;
            for (const auto & key : sectors) result[key] = type;
        };

        for (unsigned b = 0; b < dir_block; b++) mark(b, SectorType::System);
        for (unsigned b = 0; b < chain[0].total * RT11_SEGMENT_BLOCKS; b++) mark(dir_block + b, SectorType::Catalog);

        for (const auto & seg : chain)
            for (const auto & e : seg.entries) {
                SectorType type;
                if (is_live(e)) type = SectorType::File;
                else if (is_empty(e) && has_name(e)) type = SectorType::DeletedFile;
                else continue;
                for (unsigned i = 0; i < e.length; i++) mark(e.start + i, type);
            }
        return result;
    }

    // ---------------------------------------------------------------- Writing

    Result fsRT11::put_file(const UniversalFile & uf, const std::string & format, const BYTES & data, bool force_replace)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        uint16_t name[3];
        const auto name_res = make_name(uf, name);
        if (!name_res) return name_res;

        const size_t blocks = (data.size() + RT11_BLOCK_SIZE - 1) / RT11_BLOCK_SIZE;
        if (blocks >= total_blocks) return Result::error(ErrorCode::FileAddErrorSpace);
        const uint16_t needed = static_cast<uint16_t>(blocks);

        // Everything happens on a copy of the directory, which goes back to the disk only
        // once the file has found its place
        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        RT11_Entry old;
        bool replacing = false;
        for (auto & seg : chain)
            for (auto & e : seg.entries)
                if (is_live(e) && e.name[0] == name[0] && e.name[1] == name[1] && e.name[2] == name[2]) {
                    if (!force_replace) return Result::error(ErrorCode::FileAlreadyExists);
                    old = e;
                    replacing = true;
                    e.status = RT11_E_MPTY;
                }

        RT11_Entry entry;
        entry.status = RT11_E_PERM;
        std::memcpy(entry.name, name, sizeof(entry.name));
        entry.length = needed;
        entry.date = today();

        RT11_Entry src;
        if (uf.fs == FS::RT11 && entry_from_bytes(uf.metadata, src)) {
            // Copying between RT-11 disks keeps the date and the protection
            entry.date = src.date;
            entry.status = static_cast<uint16_t>(entry.status | (src.status & RT11_E_PROT));
            entry.extra = src.extra;
        } else
        if (replacing) {
            entry.status = static_cast<uint16_t>(entry.status | (old.status & RT11_E_PROT));
            entry.extra = old.extra;
        } else
        if (uf.is_protected) {
            entry.status = static_cast<uint16_t>(entry.status | RT11_E_PROT);
        }

        // A full segment is split and the search starts again
        unsigned start = 0;
        bool placed = false;
        for (unsigned attempt = 0; attempt <= RT11_MAX_SEGMENTS && !placed; attempt++) {
            for (auto & seg : chain) consolidate(seg);

            size_t si = 0, ei = 0;
            bool found = false;
            for (size_t s = 0; s < chain.size() && !found; s++)
                for (size_t i = 0; i < chain[s].entries.size(); i++) {
                    const RT11_Entry & e = chain[s].entries[i];
                    if (is_empty(e) && e.length >= needed && e.start + needed <= total_blocks) {
                        si = s;
                        ei = i;
                        found = true;
                        break;
                    }
                }
            if (!found) return Result::error(ErrorCode::FileAddErrorSpace);

            RT11_Segment & seg = chain[si];
            RT11_Entry & area = seg.entries[ei];
            entry.extra.resize(seg.extra, 0);
            entry.start = area.start;

            if (area.length == needed) {
                area = entry;
                placed = true;
            } else
            if (segment_has_room(seg)) {
                // The file takes the front of the area, what is left stays empty and nameless
                area.length = static_cast<uint16_t>(area.length - needed);
                area.start += needed;
                area.status = RT11_E_MPTY;
                area.name[0] = area.name[1] = area.name[2] = 0;
                area.date = 0;
                seg.entries.insert(seg.entries.begin() + ei, entry);
                placed = true;
            } else {
                res = split_segment(chain, si);
                if (!res) return res;
            }
            start = entry.start;
        }
        if (!placed) return Result::error(ErrorCode::FileAddErrorAllocateDirEntry);

        for (unsigned i = 0; i < needed; i++) {
            const size_t offset = static_cast<size_t>(i) * RT11_BLOCK_SIZE;
            res = write_block(start + i, data.data() + offset, data.size() - offset);
            if (!res) return res;
        }
        for (const auto & seg : chain) {
            res = save_segment(seg);
            if (!res) return res;
        }

        is_changed = true;
        stats_valid = false;
        return Result::ok();
    }

    Result fsRT11::delete_file(const UniversalFile & uf)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (uf.is_deleted) return Result::error(ErrorCode::FileDeleteError);

        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        size_t s, i;
        if (!locate(chain, uf, s, i)) return Result::error(ErrorCode::FileDeleteError);

        // The name, the length and the date stay, so that the file can be restored
        // until another one takes its place
        chain[s].entries[i].status = RT11_E_MPTY;
        res = save_segment(chain[s]);
        if (!res) return res;

        is_changed = true;
        stats_valid = false;
        return Result::ok();
    }

    Result fsRT11::restore_file(const UniversalFile & uf)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (!uf.is_deleted) return Result::ok();

        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        size_t s, i;
        if (!locate(chain, uf, s, i)) return Result::error(ErrorCode::FileRestoreError);

        RT11_Entry & e = chain[s].entries[i];
        if (name_exists(chain, e.name)) return Result::error(ErrorCode::FileAlreadyExists);

        e.status = RT11_E_PERM;
        res = save_segment(chain[s]);
        if (!res) return res;

        is_changed = true;
        stats_valid = false;
        return Result::ok();
    }

    Result fsRT11::rename_file(const UniversalFile & fd, const std::string & new_name)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (fd.is_deleted) return Result::error(ErrorCode::FileRenameError);

        uint16_t name[3];
        const auto name_res = make_name(new_name, name);
        if (!name_res) return name_res;

        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        size_t s, i;
        if (!locate(chain, fd, s, i)) return Result::error(ErrorCode::FileRenameError);

        RT11_Entry & e = chain[s].entries[i];
        if (std::memcmp(e.name, name, sizeof(name)) == 0) return Result::ok();
        if (name_exists(chain, name, &e)) return Result::error(ErrorCode::FileAlreadyExists);

        std::memcpy(e.name, name, sizeof(name));
        res = save_segment(chain[s]);
        if (!res) return res;

        is_changed = true;
        return Result::ok();
    }

    std::vector<ParameterDescription> fsRT11::file_get_metadata(const UniversalFile & fd)
    {
        RT11_Entry e;
        entry_from_bytes(fd.metadata, e);

        std::vector<ParameterDescription> params;
        params.push_back({"filename",  "{$META_FILENAME}",  ParamType::String,   fd.name});
        params.push_back({"protected", "{$META_PROTECTED}", ParamType::Checkbox, (e.status & RT11_E_PROT) ? "true" : "false"});
        params.push_back({"date",      "{$META_DATE}",      ParamType::String,   date_to_string(e.date)});
        return params;
    }

    Result fsRT11::file_set_metadata(const UniversalFile & fd, const std::map<std::string, std::string> & metadata)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (fd.is_deleted) return Result::error(ErrorCode::FileMetadataError);

        std::vector<RT11_Segment> chain;
        auto res = load_directory(chain);
        if (!res) return res;

        size_t s, i;
        if (!locate(chain, fd, s, i)) return Result::error(ErrorCode::FileMetadataError);
        RT11_Entry & e = chain[s].entries[i];

        uint16_t status = e.status;
        uint16_t date = e.date;
        std::string new_name;
        for (const auto & p : metadata) {
            if (p.first == "filename") {
                if (p.second != fd.name) new_name = p.second;
            } else
            if (p.first == "protected") {
                status = (p.second == "true") ? static_cast<uint16_t>(status | RT11_E_PROT)
                                              : static_cast<uint16_t>(status & ~RT11_E_PROT);
            } else
            if (p.first == "date") {
                if (!string_to_date(p.second, date))
                    return Result::error(ErrorCode::FileMetadataError, QT_TRANSLATE_NOOP("errors", "Invalid date"));
            }
        }

        if (status != e.status || date != e.date) {
            e.status = status;
            e.date = date;
            res = save_segment(chain[s]);
            if (!res) return res;
            is_changed = true;
        }

        if (!new_name.empty()) return rename_file(fd, new_name);
        return Result::ok();
    }

}
