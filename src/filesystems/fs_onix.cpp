// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class for the Onix (ONYX) OS filesystem, an Acorn MOS port to Agat

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_onix.h"

namespace dsk_tools {

    namespace {

        uint16_t read_word(const uint8_t * p)
        {
            return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
        }

        const char * const month_names[12] = {
            "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
        };

        // Text on these disks is KOI-8, so a byte is readable when it is either printable
        // ASCII or a Cyrillic letter. A sequential file is usually text but not always:
        // *SPOOL also captures raw VDU streams.
        bool looks_like_text(const BYTES & block, uint32_t length)
        {
            // The last block of a file is padded, so only what the file claims counts
            const size_t n = std::min(static_cast<size_t>(length), block.size());
            if (n == 0) return false;

            unsigned readable = 0;
            for (size_t i = 0; i < n; i++) {
                const uint8_t c = block[i];
                if ((c >= 0x20 && c < 0x7F) || c >= 0xC0
                    || c == 0x0D || c == 0x0A || c == 0x09) readable++;
            }
            return readable * 10 >= n * 9;
        }

        // A tokenized BBC BASIC program is a run of <CR><line hi><line lo><record length>
        // records ending with <CR><FF>. Two well formed records in a row are enough to tell
        // one from a screen dump or a text file, and that fits in the first block.
        bool looks_like_bbc_basic(const BYTES & block)
        {
            if (block.size() < 5) return false;
            if (block[0] != 0x0D) return false;
            if (block[1] == 0xFF) return false;             // an empty program

            unsigned p = 0;
            for (int line = 0; line < 2; line++) {
                if (p + 1 >= block.size()) return false;
                if (block[p] != 0x0D) return false;
                if (block[p + 1] == 0xFF) return line > 0;  // the end of the program
                if (p + 3 >= block.size()) return false;
                const unsigned len = block[p + 3];
                if (len < 4) return false;
                p += len;
            }
            return true;
        }

    }

    fsOnix::fsOnix(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsOnix::get_caps()
    {
        return FSCaps::Dirs | FSCaps::Types | FSCaps::Export;
    }

    std::string fsOnix::get_delimiter()
    {
        return ".";
    }

    // The documents on these disks are 8 bit KOI-8: Cyrillic in $C0..$FF, Latin left as
    // plain ASCII, plus the layout codes of the word processor. Program sources are a
    // different matter, see the note in dir().
    std::string fsOnix::get_charmap() const
    {
        return "onix";
    }

    // Onix numbers its blocks straight through the disk, so a block is one sector and
    // block = track * sectors_per_track + sector. Agat images take a track number running
    // through both sides and split it into a side themselves.
    Result fsOnix::read_block(unsigned block, BYTES & out) const
    {
        const unsigned spt = image->get_sectors();
        if (spt == 0 || image->get_sector_size() != ONIX_BLOCK_SIZE)
            return Result::error(ErrorCode::ReadError);

        uint8_t * p = image->get_sector_data(0, block / spt, block % spt);
        if (!p) return Result::error(ErrorCode::ReadError);

        out.assign(p, p + ONIX_BLOCK_SIZE);
        return Result::ok();
    }

    // Mirrors read_block(): where a block lives in terms of the image geometry
    bool fsOnix::block_to_hts(unsigned block, unsigned & head, unsigned & track, unsigned & sector) const
    {
        const unsigned spt = image->get_sectors();
        const unsigned tracks = image->get_tracks();
        const unsigned heads = image->get_heads();
        if (spt == 0) return false;

        unsigned track_index = block / spt;
        sector = block % spt;

        // Agat disks number their tracks through both sides, as imageAgat840 does
        if (heads == 2) {
            head = track_index & 1;
            track = track_index >> 1;
        } else {
            head = 0;
            track = track_index;
        }
        return track < tracks;
    }

    bool fsOnix::fat_is_valid(const BYTES & fat_area, unsigned total_blocks, unsigned & root_block)
    {
        root_block = 0;

        if (fat_area.size() < ONIX_FAT_BLOCKS * ONIX_BLOCK_SIZE) return false;

        const unsigned entries = ONIX_FAT_ENTRIES;
        const unsigned limit = (total_blocks != 0) ? total_blocks : entries;

        // Entry 0 is not a chain link but the first block of the root directory
        const unsigned root = read_word(fat_area.data());
        if (root < ONIX_FAT_BLOCK + ONIX_FAT_BLOCKS || root >= limit) return false;

        // The area the OS image occupies is marked $FFFF and always starts right after
        // the table itself, so a valid volume has at least the whole of track 0 reserved
        unsigned reserved = 0;
        while (reserved + 1 < entries && read_word(fat_area.data() + 2 * (reserved + 1)) == ONIX_BLOCK_SYSTEM)
            reserved++;
        if (reserved < ONIX_FAT_BLOCKS) return false;
        if (root <= reserved) return false;

        // Every remaining entry is a block number, the end of a chain, a free block or
        // reserved. Anything pointing outside the volume means this is not an Onix table.
        for (unsigned i = reserved + 1; i < entries; i++) {
            const uint16_t v = read_word(fat_area.data() + 2 * i);
            if (v == ONIX_CHAIN_END || v == ONIX_BLOCK_SYSTEM || v == ONIX_BLOCK_FREE) continue;
            if (v >= limit) return false;
        }

        root_block = root;
        return true;
    }

    bool fsOnix::dir_block_is_valid(const BYTES & block)
    {
        if (block.size() < ONIX_BLOCK_SIZE) return false;

        const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(block.data());

        if (entry->name[0] == ONIX_NAME_END) return true;       // an empty directory
        if (entry->name[0] == ONIX_NAME_DELETED) return true;

        for (unsigned i = 0; i < ONIX_NAME_LENGTH; i++) {
            const uint8_t c = entry->name[i] & 0x7F;
            if (c < 0x20 || c == 0x7F) return false;
        }
        return entry_type(*entry) != 0;
    }

    bool fsOnix::volume_is_valid(const BYTES & image, unsigned total_blocks)
    {
        const unsigned fat_start = ONIX_FAT_BLOCK * ONIX_BLOCK_SIZE;
        const unsigned fat_end = fat_start + ONIX_FAT_BLOCKS * ONIX_BLOCK_SIZE;
        if (image.size() < fat_end) return false;

        unsigned root = 0;
        const BYTES fat_area(image.begin() + fat_start, image.begin() + fat_end);
        if (!fat_is_valid(fat_area, total_blocks, root)) return false;

        const unsigned root_start = root * ONIX_BLOCK_SIZE;
        if (image.size() < root_start + ONIX_BLOCK_SIZE) return false;

        const BYTES root_data(image.begin() + root_start, image.begin() + root_start + ONIX_BLOCK_SIZE);
        return dir_block_is_valid(root_data);
    }

    Result fsOnix::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        if (image->get_sector_size() != ONIX_BLOCK_SIZE)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Onix: unsupported disk geometry"));

        disk_blocks = image->get_heads() * image->get_tracks() * image->get_sectors();
        if (disk_blocks <= ONIX_FAT_BLOCK + ONIX_FAT_BLOCKS)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Onix: unsupported disk geometry"));

        BYTES fat_area;
        fat_area.reserve(ONIX_FAT_BLOCKS * ONIX_BLOCK_SIZE);
        for (unsigned i = 0; i < ONIX_FAT_BLOCKS; i++) {
            BYTES block;
            auto res = read_block(ONIX_FAT_BLOCK + i, block);
            if (!res) return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Onix allocation table not found"));
            fat_area.insert(fat_area.end(), block.begin(), block.end());
        }

        if (!fat_is_valid(fat_area, disk_blocks, root_block))
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Onix allocation table not found"));

        m_fat.resize(ONIX_FAT_ENTRIES);
        for (unsigned i = 0; i < ONIX_FAT_ENTRIES; i++)
            m_fat[i] = read_word(fat_area.data() + 2 * i);

        current_path.clear();
        current_path.push_back(root_block);

        is_open = true;
        return Result::ok();
    }

    // The table describes the first ONIX_FAT_ENTRIES blocks only. A chain reaching beyond
    // that stops there instead of reading the OS image as if it were a table.
    void fsOnix::block_chain(unsigned start, std::vector<unsigned> & blocks) const
    {
        blocks.clear();

        std::set<unsigned> visited;
        unsigned block = start;

        while (block != ONIX_BLOCK_FREE && block < disk_blocks
               && block != ONIX_CHAIN_END && block != ONIX_BLOCK_SYSTEM) {
            if (!visited.insert(block).second) break;           // a looped chain
            blocks.push_back(block);
            if (block >= m_fat.size()) break;                   // outside the table
            block = m_fat[block];
        }
    }

    void fsOnix::cd_up()
    {
        if (current_path.size() > 1)
            current_path.pop_back();
    }

    void fsOnix::cd(const UniversalFile & dir, bool & updir)
    {
        if (dir.name == "..") {
            cd_up();
            updir = true;
        } else {
            if (dir.metadata.size() >= sizeof(Onix_Dir_Entry)) {
                const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(dir.metadata.data());
                if (entry->start_block != 0 && entry->start_block < disk_blocks)
                    current_path.push_back(entry->start_block);
            }
            updir = false;
        }
    }

    bool fsOnix::is_root()
    {
        return current_path.size() <= 1;
    }

    // Walks the chain of blocks of one directory. Entries do not span a block boundary:
    // each block holds ONIX_ENTRIES_PER_BLK of them and a name starting with a zero ends
    // the entries of that block only, the chain goes on.
    Result fsOnix::read_directory(unsigned first_block, std::vector<BYTES> & entries,
                                  std::vector<unsigned> & entry_blocks, std::vector<unsigned> & entry_slots) const
    {
        entries.clear();
        entry_blocks.clear();
        entry_slots.clear();

        std::vector<unsigned> blocks;
        block_chain(first_block, blocks);

        for (unsigned block : blocks) {
            BYTES data;
            auto res = read_block(block, data);
            if (!res) return res;

            for (unsigned i = 0; i < ONIX_ENTRIES_PER_BLK; i++) {
                const unsigned offset = i * ONIX_ENTRY_LENGTH;
                if (data[offset] == ONIX_NAME_END) break;
                entries.push_back(BYTES(data.begin() + offset, data.begin() + offset + ONIX_ENTRY_LENGTH));
                entry_blocks.push_back(block);
                entry_slots.push_back(i);
            }
        }

        return Result::ok();
    }

    // Where the length of a file is kept depends on how it was made. The OS branches on
    // the very same bits: LDA attr / AND #$C0 / CMP #$80, and then takes the length from
    // the first word instead of the second one.
    uint32_t fsOnix::entry_length(const Onix_Dir_Entry & entry)
    {
        switch (entry_type(entry)) {
            case ONIX_TYPE_SEQ:  return entry.word_a;
            case ONIX_TYPE_FILE: return entry.word_b;
            default:             return 0;                  // a directory has no length
        }
    }

    std::string fsOnix::entry_name(const Onix_Dir_Entry & entry)
    {
        std::string name;
        for (unsigned i = 0; i < ONIX_NAME_LENGTH; i++)
            name += static_cast<char>(entry.name[i] & 0x7F);
        return trim(name);
    }

    std::string fsOnix::type_label(const Onix_Dir_Entry & entry)
    {
        switch (entry_type(entry)) {
            case ONIX_TYPE_DIR:  return "DIR";
            case ONIX_TYPE_FILE: return "FIL";
            case ONIX_TYPE_SEQ:  return "SEQ";
            default:             return "$" + int_to_hex(entry.attributes);
        }
    }

    // Onix keeps a day and a month, which is all *CAT prints; the bits left over in both
    // bytes look like a year but no sample carries one to check against
    std::string fsOnix::entry_date(const Onix_Dir_Entry & entry)
    {
        const unsigned month = entry.month & 0x0F;
        const unsigned day = entry.day & 0x1F;

        if (month == 0 || month > 12 || day == 0) return "<{$NO_DATE}>";

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02u %s", day, month_names[month - 1]);
        return std::string(buffer);
    }

    Result fsOnix::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const unsigned first_block = current_path.empty() ? root_block : current_path.back();

        if (current_path.size() > 1) {
            UniversalFile updir{};
            updir.fs = get_fs();
            updir.name = "..";
            updir.is_dir = true;
            updir.is_deleted = false;
            files.push_back(updir);
        }

        std::vector<BYTES> entries;
        std::vector<unsigned> entry_blocks;
        std::vector<unsigned> entry_slots;
        auto res = read_directory(first_block, entries, entry_blocks, entry_slots);
        if (!res) return res;

        for (size_t i = 0; i < entries.size(); i++) {
            const uint8_t * raw = entries[i].data();

            // A deleted entry keeps no name, so there is nothing worth showing
            if (raw[0] == ONIX_NAME_DELETED) continue;

            const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(raw);
            if (entry_type(*entry) == 0) continue;

            UniversalFile f{};
            f.fs = get_fs();
            f.name = entry_name(*entry);
            f.is_dir = entry_type(*entry) == ONIX_TYPE_DIR;
            f.is_deleted = false;
            f.is_protected = false;
            f.size = entry_length(*entry);
            f.attributes = entry->attributes;
            f.type_label = type_label(*entry);

            // A BASIC program written under *RUS keeps its Cyrillic as 7 bit KOI-7, where
            // the same bytes are also ordinary Latin letters, so only the reader can tell
            // the two apart and the viewer is left on the volume default.
            f.type_preferred = PreferredType::Binary;
            BYTES head;
            if (entry->start_block < disk_blocks && read_block(entry->start_block, head)) {
                if (entry_type(*entry) == ONIX_TYPE_FILE && looks_like_bbc_basic(head))
                    f.type_preferred = PreferredType::BBCBasic;
                else
                if (looks_like_text(head, f.size))
                    f.type_preferred = PreferredType::Text;
            }

            f.original_name.assign(entry->name, entry->name + ONIX_NAME_LENGTH);

            f.metadata.resize(sizeof(Onix_Dir_Entry));
            std::memcpy(f.metadata.data(), raw, sizeof(Onix_Dir_Entry));

            f.position.push_back(entry_blocks[i]);
            f.position.push_back(entry_slots[i]);

            files.push_back(f);
        }

        return Result::ok();
    }

    Result fsOnix::find_file(const std::string & file_name, UniversalFile & fd)
    {
        Files files;
        auto res = dir(files, false);
        if (!res) return res;

        // Onix keeps the case a name was typed in but does not tell names apart by it
        for (const UniversalFile & f : files) {
            if (to_upper(f.name) == to_upper(file_name)) {
                fd = f;
                return Result::ok();
            }
        }
        return Result::error(ErrorCode::NotFound);
    }

    Result fsOnix::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (uf.metadata.size() < sizeof(Onix_Dir_Entry))
            return Result::error(ErrorCode::FileIncorrectFS);

        const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(uf.metadata.data());

        data.clear();

        std::vector<unsigned> blocks;
        block_chain(entry->start_block, blocks);

        const uint32_t length = entry_length(*entry);
        data.reserve(length);

        for (unsigned block : blocks) {
            if (data.size() >= length) break;
            BYTES block_data;
            auto res = read_block(block, block_data);
            if (!res) return res;
            data.insert(data.end(), block_data.begin(), block_data.end());
        }

        if (data.size() > length) data.resize(length);

        return Result::ok();
    }

    // Blocks the table refuses to hand out: the boot block, the table itself and the
    // area the OS image occupies, which every sample marks with $FFFF in one run
    unsigned fsOnix::system_blocks() const
    {
        unsigned count = 1;                                 // the boot block
        for (unsigned i = 1; i < m_fat.size(); i++)
            if (m_fat[i] == ONIX_BLOCK_SYSTEM) count++;
        return count;
    }

    // A block that belongs to no file reads back as zero, both when it has never been
    // handed out and when a deleted file gave it back
    unsigned fsOnix::free_blocks() const
    {
        unsigned count = 0;
        for (unsigned i = 1; i < m_fat.size(); i++)
            if (m_fat[i] == ONIX_BLOCK_FREE) count++;
        return count;
    }

    void fsOnix::update_stats()
    {
        m_stats.int_values.clear();

        const unsigned image_size = image->get_heads() * image->get_tracks()
                                  * image->get_sectors() * image->get_sector_size();
        m_stats.int_values["image_size"] = image_size;

        if (is_open) {
            const unsigned described = static_cast<unsigned>(m_fat.size());
            const unsigned reserved = system_blocks();
            const unsigned data = (described > reserved) ? (described - reserved) : 0;
            const unsigned free = free_blocks();

            m_stats.int_values["total_space"]    = data * ONIX_BLOCK_SIZE;
            m_stats.int_values["occupied_space"] = (data - free) * ONIX_BLOCK_SIZE;
            m_stats.int_values["free_space"]     = free * ONIX_BLOCK_SIZE;
        } else {
            m_stats.int_values["total_space"]    = 0;
            m_stats.int_values["occupied_space"] = 0;
            m_stats.int_values["free_space"]     = 0;
        }

        stats_valid = true;
    }

    std::string fsOnix::information()
    {
        if (!is_open) return "";

        std::string result;

        const unsigned described = static_cast<unsigned>(m_fat.size());
        const unsigned reserved = system_blocks();
        const unsigned free = free_blocks();
        const unsigned data = (described > reserved) ? (described - reserved) : 0;

        result += "{$ONIX_LAYOUT}:\n";
        result += "    {$ONIX_BLOCK_SIZE}: " + std::to_string(ONIX_BLOCK_SIZE) + " {$BYTES}\n";
        result += "    {$ONIX_DISK_BLOCKS}: " + std::to_string(disk_blocks) + "\n";
        result += "    {$ONIX_DESCRIBED_BLOCKS}: " + std::to_string(described) + "\n";
        result += "    {$ONIX_SYSTEM_BLOCKS}: " + std::to_string(reserved) + "\n";
        result += "    {$ONIX_USED_BLOCKS}: " + std::to_string(data - free) + "\n";
        result += "    {$ONIX_FREE_BLOCKS}: " + std::to_string(free) + "\n";
        result += "    {$FREE_BYTES}: " + std::to_string(static_cast<uint64_t>(free) * ONIX_BLOCK_SIZE) + "\n";
        result += "\n";

        result += "{$ONIX_ALLOCATION_TABLE}:\n";
        result += "    {$ONIX_TABLE_FIRST_BLOCK}: " + std::to_string(ONIX_FAT_BLOCK) + "\n";
        result += "    {$ONIX_TABLE_BLOCKS}: " + std::to_string(ONIX_FAT_BLOCKS) + "\n";
        result += "    {$ONIX_ROOT_BLOCK}: " + std::to_string(root_block) + "\n";
        result += "\n";

        result += "{$ONIX_DIRECTORY}:\n";
        result += "    {$ONIX_ENTRY_LENGTH}: " + std::to_string(ONIX_ENTRY_LENGTH) + "\n";
        result += "    {$ONIX_ENTRIES_PER_BLOCK}: " + std::to_string(ONIX_ENTRIES_PER_BLK) + "\n";

        return result;
    }

    std::string fsOnix::file_info(const UniversalFile & fd)
    {
        if (fd.metadata.size() < sizeof(Onix_Dir_Entry)) return "";

        const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(fd.metadata.data());

        std::string result;
        result += "{$DIRECTORY_ENTRY}:\n";
        result += "    {$FILE_NAME}: " + fd.name + " (" + toHexList(entry->name, ONIX_NAME_LENGTH, "$") + ")\n";
        result += "    {$TYPE}: " + type_label(*entry) + "\n";
        result += "    {$ONIX_ATTRIBUTES}: $" + int_to_hex(entry->attributes) + "\n";
        result += "    {$ONIX_DATE}: " + entry_date(*entry) + "\n";
        result += "    {$ONIX_FIRST_BLOCK}: " + std::to_string(entry->start_block) + "\n";

        if (entry_type(*entry) != ONIX_TYPE_DIR) {
            result += "    {$SIZE}: " + std::to_string(entry_length(*entry)) + " {$BYTES}\n";

            // A sequential file is written through a channel, so it carries no addresses
            if (entry_type(*entry) == ONIX_TYPE_FILE) {
                result += "    {$ONIX_LOAD_ADDRESS}: $" + int_to_hex(entry->word_a) + "\n";
                result += "    {$ONIX_EXEC_ADDRESS}: $" + int_to_hex(entry->word_c) + "\n";
            }
        }

        std::vector<unsigned> blocks;
        block_chain(entry->start_block, blocks);

        result += "\n{$ONIX_FILE_BLOCKS}:\n    ";
        size_t shown = 0;
        for (unsigned block : blocks) {
            result += std::to_string(block) + " ";
            if (++shown % 16 == 0) result += "\n    ";
        }
        result += "\n";

        return result;
    }

    std::vector<std::string> fsOnix::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    SectorTypeMap fsOnix::get_sector_type_map()
    {
        SectorTypeMap result;
        if (!is_open) return result;

        auto mark_block = [&](unsigned block, SectorType type, bool overwrite_any) {
            unsigned h, t, s;
            if (block >= disk_blocks) return;
            if (!block_to_hts(block, h, t, s)) return;
            const std::array<unsigned, 3> key{h, t, s};
            const auto it = result.find(key);
            if (it == result.end() || overwrite_any) result[key] = type;
        };

        mark_block(ONIX_BOOT_BLOCK, SectorType::System, false);
        for (unsigned i = 0; i < ONIX_FAT_BLOCKS; i++)
            mark_block(ONIX_FAT_BLOCK + i, SectorType::Catalog, false);

        // The OS image itself, everything the table refuses to hand out
        for (unsigned i = 1; i < m_fat.size(); i++)
            if (m_fat[i] == ONIX_BLOCK_SYSTEM) mark_block(i, SectorType::System, false);

        // Every directory reachable from the root one, and the files in it
        std::vector<unsigned> dir_queue;
        std::set<unsigned> visited;
        dir_queue.push_back(root_block);
        visited.insert(root_block);

        while (!dir_queue.empty()) {
            const unsigned first_block = dir_queue.back();
            dir_queue.pop_back();

            std::vector<BYTES> entries;
            std::vector<unsigned> entry_blocks;
            std::vector<unsigned> entry_slots;
            if (!read_directory(first_block, entries, entry_blocks, entry_slots)) continue;

            for (unsigned block : entry_blocks) mark_block(block, SectorType::Catalog, true);

            for (const auto & raw : entries) {
                if (raw[0] == ONIX_NAME_DELETED) continue;

                const auto * entry = reinterpret_cast<const Onix_Dir_Entry *>(raw.data());
                if (entry_type(*entry) == 0) continue;

                if (entry_type(*entry) == ONIX_TYPE_DIR) {
                    if (entry->start_block != 0 && entry->start_block < disk_blocks
                        && visited.insert(entry->start_block).second)
                        dir_queue.push_back(entry->start_block);
                    continue;
                }

                std::vector<unsigned> blocks;
                block_chain(entry->start_block, blocks);
                for (unsigned block : blocks) mark_block(block, SectorType::File, false);
            }
        }

        return result;
    }

}
