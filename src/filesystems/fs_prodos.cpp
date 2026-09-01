// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the ProDOS filesystem (Agat Nippel OS, Apple II)

#include <cstdio>
#include <cstring>
#include <set>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_prodos.h"

namespace dsk_tools {

    namespace {

        uint16_t read_word(const uint8_t * p)
        {
            return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
        }

        uint8_t storage_type(const uint8_t * entry)
        {
            return entry[0] >> 4;
        }

        // A block number pointing outside the volume means a damaged or alien directory
        bool block_is_sane(unsigned block, unsigned limit)
        {
            return block != 0 && block < limit;
        }

    }

    fsProDOS::fsProDOS(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsProDOS::get_caps()
    {
        return FSCaps::Dirs | FSCaps::Types | FSCaps::Protect | FSCaps::Export;
    }

    std::string fsProDOS::get_delimiter()
    {
        return "/";
    }

    // A ProDOS block is 512 bytes, which is either one sector of an 880 Kb Agat disk
    // or two consecutive sectors of a 256 byte per sector image stored in ProDOS order
    Result fsProDOS::read_block(unsigned block, BYTES & out) const
    {
        const unsigned sector_size = image->get_sector_size();
        const unsigned spt = image->get_sectors();
        if (sector_size == 0 || spt == 0 || PRODOS_BLOCK_SIZE % sector_size != 0)
            return Result::error(ErrorCode::ReadError);

        const unsigned spb = PRODOS_BLOCK_SIZE / sector_size;

        out.resize(PRODOS_BLOCK_SIZE);
        for (unsigned i = 0; i < spb; i++) {
            const unsigned ls = block * spb + i;             // linear sector number
            uint8_t * p = image->get_sector_data(0, ls / spt, ls % spt);
            if (!p) return Result::error(ErrorCode::ReadError);
            std::memcpy(out.data() + i * sector_size, p, sector_size);
        }
        return Result::ok();
    }

    // Mirrors read_block(): where a block lives in terms of the image geometry.
    // `index` selects one of the sectors a block is made of.
    bool fsProDOS::block_to_hts(unsigned block, unsigned index, unsigned & head, unsigned & track, unsigned & sector) const
    {
        const unsigned sector_size = image->get_sector_size();
        const unsigned spt = image->get_sectors();
        const unsigned tracks = image->get_tracks();
        const unsigned heads = image->get_heads();
        if (sector_size == 0 || spt == 0 || PRODOS_BLOCK_SIZE % sector_size != 0) return false;

        const unsigned ls = block * (PRODOS_BLOCK_SIZE / sector_size) + index;
        unsigned track_index = ls / spt;
        sector = ls % spt;

        // Agat disks number their tracks through both sides, as imageAgat880 does
        if (heads == 2) {
            head = track_index & 1;
            track = track_index >> 1;
        } else {
            head = 0;
            track = track_index;
        }
        return track < tracks;
    }

    bool fsProDOS::volume_header_is_valid(const BYTES & block, unsigned disk_blocks)
    {
        if (block.size() < PRODOS_BLOCK_SIZE) return false;

        // The volume directory is the head of its chain
        if (read_word(block.data()) != 0) return false;

        const uint8_t * entry = block.data() + PRODOS_DIR_HEADER_LEN;
        if (storage_type(entry) != PRODOS_ST_VOLUME_HDR) return false;

        const auto * VH = reinterpret_cast<const ProDOS_Volume_Header *>(entry);
        if ((VH->storage_name_length & 0x0F) == 0) return false;
        if (VH->entry_length != PRODOS_ENTRY_LENGTH) return false;
        if (VH->entries_per_block != PRODOS_ENTRIES_PER_BLK) return false;
        if (VH->total_blocks == 0) return false;
        if (disk_blocks != 0 && VH->total_blocks > disk_blocks) return false;
        if (!block_is_sane(VH->bit_map_pointer, VH->total_blocks)) return false;

        // A volume name is made of characters, on Agat disks with the high bit set at will
        for (int i = 0; i < (VH->storage_name_length & 0x0F); i++) {
            const uint8_t c = VH->file_name[i] & 0x7F;
            if (c < 0x20) return false;
        }
        return true;
    }

    Result fsProDOS::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        const unsigned sector_size = image->get_sector_size();
        if (sector_size == 0 || PRODOS_BLOCK_SIZE % sector_size != 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "ProDOS: unsupported sector size"));

        sectors_per_block = PRODOS_BLOCK_SIZE / sector_size;
        disk_blocks = image->get_heads() * image->get_tracks() * image->get_sectors() / sectors_per_block;

        BYTES block;
        auto res = read_block(PRODOS_ROOT_BLOCK, block);
        if (!res) return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Cannot read ProDOS volume directory"));

        if (!volume_header_is_valid(block, disk_blocks))
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "ProDOS volume header not found"));

        std::memcpy(&VH, block.data() + PRODOS_DIR_HEADER_LEN, sizeof(VH));

        entry_length = VH.entry_length;
        entries_per_block = VH.entries_per_block;
        total_blocks = VH.total_blocks;

        current_path.clear();
        current_path.push_back(PRODOS_ROOT_BLOCK);

        is_open = true;
        return Result::ok();
    }

    void fsProDOS::cd_up()
    {
        if (current_path.size() > 1)
            current_path.pop_back();
    }

    void fsProDOS::cd(const UniversalFile & dir, bool & updir)
    {
        if (dir.name == "..") {
            cd_up();
            updir = true;
        } else {
            if (dir.metadata.size() >= sizeof(ProDOS_File_Entry)) {
                const auto * entry = reinterpret_cast<const ProDOS_File_Entry *>(dir.metadata.data());
                if (block_is_sane(entry->key_pointer, total_blocks))
                    current_path.push_back(entry->key_pointer);
            }
            updir = false;
        }
    }

    bool fsProDOS::is_root()
    {
        return current_path.size() <= 1;
    }

    // Walks the chain of blocks of one directory and returns every entry of it,
    // together with the block each entry was found in
    Result fsProDOS::read_directory(unsigned key_block, std::vector<BYTES> & entries, std::vector<unsigned> & entry_blocks) const
    {
        entries.clear();
        entry_blocks.clear();

        unsigned block = key_block;
        std::set<unsigned> visited;

        while (block_is_sane(block, total_blocks)) {
            if (!visited.insert(block).second) break;       // a looped chain

            BYTES data;
            auto res = read_block(block, data);
            if (!res) return res;

            for (unsigned i = 0; i < entries_per_block; i++) {
                const unsigned offset = PRODOS_DIR_HEADER_LEN + i * entry_length;
                if (offset + entry_length > PRODOS_BLOCK_SIZE) break;
                entries.push_back(BYTES(data.begin() + offset, data.begin() + offset + entry_length));
                entry_blocks.push_back(block);
            }

            block = read_word(data.data() + 2);             // next block of the directory
        }

        return Result::ok();
    }

    std::string fsProDOS::entry_name(const uint8_t * entry)
    {
        const int len = entry[0] & 0x0F;

        // Agat writes names in its own character set, where the high bit makes a letter
        // upper case, so JukyDOS shows up the way it was typed in. Apple II keeps to
        // plain ASCII with the high bit always clear, and that is what tells them apart.
        bool agat_charset = false;
        for (int i=0; i<len; i++)
            if (entry[1 + i] & 0x80) agat_charset = true;

        if (agat_charset) return trim(agat_to_utf(entry + 1, len));

        return trim(std::string(reinterpret_cast<const char *>(entry + 1), len));
    }

    std::string fsProDOS::type_label(uint8_t file_type)
    {
        switch (file_type) {
            case 0x00: return "";
            case 0x01: return "BAD";
            case 0x02: return "PCD";
            case 0x03: return "PTX";
            case 0x04: return "TXT";
            case 0x05: return "PDA";
            case 0x06: return "BIN";
            case 0x07: return "FNT";
            case 0x08: return "FOT";
            case 0x09: return "BA3";
            case 0x0A: return "DA3";
            case 0x0B: return "WPF";
            case 0x0C: return "SOS";
            case 0x0F: return "DIR";
            case 0x19: return "ADB";
            case 0x1A: return "AWP";
            case 0x1B: return "ASP";
            case 0xEF: return "PAS";
            case 0xF0: return "CMD";
            case 0xFA: return "INT";
            case 0xFB: return "IVR";
            case 0xFC: return "BAS";
            case 0xFD: return "VAR";
            case 0xFE: return "REL";
            case 0xFF: return "SYS";
            default:   return "$" + int_to_hex(file_type);
        }
    }

    // ProDOS date: yyyyyyym mmmddddd, time: 000hhhhh 00mmmmmm
    std::string fsProDOS::date_time(uint16_t date, uint16_t time)
    {
        if (date == 0) return "<{$NO_DATE}>";

        const unsigned day   = date & 0x1F;
        const unsigned month = (date >> 5) & 0x0F;
        const unsigned year  = (date >> 9) & 0x7F;
        const unsigned hour  = (time >> 8) & 0x1F;
        const unsigned min   = time & 0x3F;

        // ProDOS counts years from 1900, with 00..39 usually meaning the 2000s
        const unsigned full_year = (year < 40) ? (2000 + year) : (1900 + year);

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u", full_year, month, day, hour, min);
        return std::string(buffer);
    }

    Result fsProDOS::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const unsigned key_block = current_path.empty() ? PRODOS_ROOT_BLOCK : current_path.back();

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
        auto res = read_directory(key_block, entries, entry_blocks);
        if (!res) return res;

        for (size_t i = 0; i < entries.size(); i++) {
            const uint8_t * raw = entries[i].data();
            const uint8_t st = storage_type(raw);

            // The first entry of any directory describes the directory itself.
            // A deleted entry has its storage type and name length zeroed out, and
            // ProDOS keeps no way back, so there is nothing to show for it.
            if (st == PRODOS_ST_VOLUME_HDR || st == PRODOS_ST_SUBDIR_HDR) continue;
            if (st == PRODOS_ST_DELETED) continue;

            const auto * entry = reinterpret_cast<const ProDOS_File_Entry *>(raw);

            UniversalFile f{};
            f.fs = get_fs();
            f.name = entry_name(raw);
            f.is_dir = (st == PRODOS_ST_SUBDIR);
            f.is_deleted = false;
            f.is_protected = (entry->access & PRODOS_ACCESS_WRITE) == 0;
            f.attributes = entry->access;

            // For an extended file the size shown is the one of the data fork, which is
            // also what gets exported; the resource fork is reported in the file info
            Fork data_fork;
            read_fork(*entry, false, data_fork);
            f.size = data_fork.present
                   ? data_fork.eof
                   : (static_cast<uint32_t>(entry->eof[0])
                      | (static_cast<uint32_t>(entry->eof[1]) << 8)
                      | (static_cast<uint32_t>(entry->eof[2]) << 16));

            f.type_label = f.is_dir ? "DIR" : type_label(entry->file_type);

            if (entry->file_type == PRODOS_TYPE_TEXT)
                f.type_preferred = PreferredType::Text;
            else
            if (entry->file_type == PRODOS_TYPE_APPLESOFT)
                f.type_preferred = PreferredType::AppleBASIC;
            else
                f.type_preferred = PreferredType::Binary;

            f.original_name.assign(raw + 1, raw + 1 + (raw[0] & 0x0F));

            f.metadata.resize(sizeof(ProDOS_File_Entry));
            std::memcpy(f.metadata.data(), raw, sizeof(ProDOS_File_Entry));

            f.position.push_back(entry_blocks[i]);
            f.position.push_back(static_cast<uint32_t>(i % entries_per_block));

            files.push_back(f);
        }

        return Result::ok();
    }

    Result fsProDOS::find_file(const std::string & file_name, UniversalFile & fd)
    {
        Files files;
        auto res = dir(files, false);
        if (!res) return res;

        for (const UniversalFile & f : files) {
            if (to_upper(f.name) == file_name) {
                fd = f;
                return Result::ok();
            }
        }
        return Result::error(ErrorCode::NotFound);
    }

    // Where the data of a file begins. An ordinary file says it in its directory entry;
    // an extended one (GS/OS) points at a key block holding two mini entries instead:
    // the data fork at offset 0 and the resource fork at offset $100.
    Result fsProDOS::read_fork(const ProDOS_File_Entry & entry, bool resource, Fork & fork) const
    {
        fork = Fork();

        const uint8_t st = entry.storage_name_length >> 4;

        if (st != PRODOS_ST_EXTENDED) {
            if (resource) return Result::ok();              // no forks, nothing to report
            fork.storage_type = st;
            fork.key_pointer = entry.key_pointer;
            fork.eof = static_cast<uint32_t>(entry.eof[0])
                     | (static_cast<uint32_t>(entry.eof[1]) << 8)
                     | (static_cast<uint32_t>(entry.eof[2]) << 16);
            fork.present = true;
            return Result::ok();
        }

        if (!block_is_sane(entry.key_pointer, total_blocks)) return Result::error(ErrorCode::ReadError);

        BYTES key_block;
        auto res = read_block(entry.key_pointer, key_block);
        if (!res) return res;

        const auto * mini = reinterpret_cast<const ProDOS_Fork_Entry *>(
            key_block.data() + (resource ? PRODOS_RESOURCE_FORK_OFFSET : 0));

        fork.storage_type = mini->storage_type & 0x0F;
        fork.key_pointer = mini->key_pointer;
        fork.eof = static_cast<uint32_t>(mini->eof[0])
                 | (static_cast<uint32_t>(mini->eof[1]) << 8)
                 | (static_cast<uint32_t>(mini->eof[2]) << 16);
        fork.present = fork.storage_type != 0;

        return Result::ok();
    }

    // Collects the data blocks of a fork in order. A zero pointer means a sparse
    // hole, which reads back as a block of zeroes, so it is kept in the list.
    Result fsProDOS::file_blocks(const Fork & fork, std::vector<unsigned> & blocks) const
    {
        blocks.clear();

        const uint8_t st = fork.storage_type;
        const unsigned key = fork.key_pointer;

        if (!fork.present) return Result::ok();             // an empty fork has no blocks

        auto read_index = [&](unsigned index_block, std::vector<unsigned> & out) -> Result {
            BYTES data;
            auto res = read_block(index_block, data);
            if (!res) return res;
            out.clear();
            for (unsigned i = 0; i < PRODOS_INDEX_ENTRIES; i++)
                out.push_back(static_cast<unsigned>(data[i]) | (static_cast<unsigned>(data[i + PRODOS_INDEX_ENTRIES]) << 8));
            return Result::ok();
        };

        if (st == PRODOS_ST_SEEDLING) {
            if (!block_is_sane(key, total_blocks)) return Result::error(ErrorCode::ReadError);
            blocks.push_back(key);
            return Result::ok();
        }

        if (st == PRODOS_ST_SAPLING) {
            if (!block_is_sane(key, total_blocks)) return Result::error(ErrorCode::ReadError);
            return read_index(key, blocks);
        }

        if (st == PRODOS_ST_TREE) {
            if (!block_is_sane(key, total_blocks)) return Result::error(ErrorCode::ReadError);
            std::vector<unsigned> index_blocks;
            auto res = read_index(key, index_blocks);
            if (!res) return res;
            for (unsigned index_block : index_blocks) {
                if (index_block == 0) {
                    // A sparse hole covering a whole index block
                    blocks.insert(blocks.end(), PRODOS_INDEX_ENTRIES, 0);
                    continue;
                }
                if (!block_is_sane(index_block, total_blocks)) return Result::error(ErrorCode::ReadError);
                std::vector<unsigned> data_blocks;
                res = read_index(index_block, data_blocks);
                if (!res) return res;
                blocks.insert(blocks.end(), data_blocks.begin(), data_blocks.end());
            }
            return Result::ok();
        }

        return Result::error(ErrorCode::NotImplementedYet, QT_TRANSLATE_NOOP("errors", "ProDOS: unsupported file storage type"));
    }

    Result fsProDOS::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (uf.metadata.size() < sizeof(ProDOS_File_Entry))
            return Result::error(ErrorCode::FileIncorrectFS);

        const auto * entry = reinterpret_cast<const ProDOS_File_Entry *>(uf.metadata.data());

        data.clear();

        // An extended file exports its data fork
        Fork fork;
        auto res = read_fork(*entry, false, fork);
        if (!res) return res;

        std::vector<unsigned> blocks;
        res = file_blocks(fork, blocks);
        if (!res) return res;

        const uint32_t eof = fork.eof;

        data.reserve(eof);

        for (unsigned block : blocks) {
            if (data.size() >= eof) break;
            if (block == 0) {
                // A sparse hole reads back as zeroes
                data.insert(data.end(), PRODOS_BLOCK_SIZE, 0);
                continue;
            }
            if (!block_is_sane(block, total_blocks)) return Result::error(ErrorCode::ReadError);
            BYTES block_data;
            res = read_block(block, block_data);
            if (!res) return res;
            data.insert(data.end(), block_data.begin(), block_data.end());
        }

        if (data.size() > eof) data.resize(eof);

        return Result::ok();
    }

    unsigned fsProDOS::free_blocks() const
    {
        if (!is_open) return 0;

        // The bitmap holds one bit per block, most significant bit first, a set bit is free
        const unsigned bitmap_blocks = (total_blocks + PRODOS_BLOCK_SIZE * 8 - 1) / (PRODOS_BLOCK_SIZE * 8);
        unsigned free = 0;
        unsigned block_no = 0;

        for (unsigned i = 0; i < bitmap_blocks; i++) {
            BYTES data;
            if (!read_block(VH.bit_map_pointer + i, data)) return free;
            for (unsigned byte = 0; byte < PRODOS_BLOCK_SIZE && block_no < total_blocks; byte++) {
                for (int bit = 7; bit >= 0 && block_no < total_blocks; bit--, block_no++) {
                    if ((data[byte] >> bit) & 1) free++;
                }
            }
        }
        return free;
    }

    std::string fsProDOS::information()
    {
        if (!is_open) return "";

        std::string result;

        const unsigned free = free_blocks();

        result += "{$PRODOS_VOLUME_NAME}: " + entry_name(reinterpret_cast<const uint8_t *>(&VH))
                + " (" + toHexList(VH.file_name, VH.storage_name_length & 0x0F, "$") + ")\n";
        result += "{$PRODOS_CREATED}: " + date_time(VH.creation_date, VH.creation_time) + "\n";
        result += "{$PRODOS_VERSION}: " + std::to_string(VH.version)
                + " ({$PRODOS_MIN_VERSION}: " + std::to_string(VH.min_version) + ")\n";
        result += "{$PRODOS_ACCESS}: $" + int_to_hex(VH.access) + "\n";
        result += "\n";

        result += "{$PRODOS_LAYOUT}:\n";
        result += "    {$PRODOS_BLOCK_SIZE}: " + std::to_string(PRODOS_BLOCK_SIZE) + " {$BYTES}\n";
        result += "    {$PRODOS_TOTAL_BLOCKS}: " + std::to_string(total_blocks) + "\n";
        result += "    {$PRODOS_USED_BLOCKS}: " + std::to_string(total_blocks - free) + "\n";
        result += "    {$PRODOS_FREE_BLOCKS}: " + std::to_string(free) + "\n";
        result += "    {$FREE_BYTES}: " + std::to_string(static_cast<uint64_t>(free) * PRODOS_BLOCK_SIZE) + "\n";
        result += "    {$PRODOS_BITMAP_POINTER}: " + std::to_string(VH.bit_map_pointer) + "\n";
        result += "\n";

        result += "{$PRODOS_DIRECTORY}:\n";
        result += "    {$PRODOS_FILE_COUNT}: " + std::to_string(VH.file_count) + "\n";
        result += "    {$PRODOS_ENTRY_LENGTH}: " + std::to_string(VH.entry_length) + "\n";
        result += "    {$PRODOS_ENTRIES_PER_BLOCK}: " + std::to_string(VH.entries_per_block) + "\n";

        return result;
    }

    std::string fsProDOS::file_info(const UniversalFile & fd)
    {
        if (fd.metadata.size() < sizeof(ProDOS_File_Entry)) return "";

        const auto * entry = reinterpret_cast<const ProDOS_File_Entry *>(fd.metadata.data());
        const uint8_t st = entry->storage_name_length >> 4;

        std::string result;
        result += "{$DIRECTORY_ENTRY}:\n";
        result += "    {$FILE_NAME}: " + fd.name + " (" + toHexList(entry->file_name, entry->storage_name_length & 0x0F, "$") + ")\n";
        result += "    {$TYPE}: " + type_label(entry->file_type) + " ($" + int_to_hex(entry->file_type) + ")\n";
        result += "    {$PRODOS_STORAGE_TYPE}: " + std::to_string(st) + "\n";
        result += "    {$SIZE}: " + std::to_string(fd.size) + " {$BYTES}\n";
        result += "    {$PRODOS_BLOCKS_USED}: " + std::to_string(entry->blocks_used) + "\n";
        result += "    {$PRODOS_KEY_POINTER}: " + std::to_string(entry->key_pointer) + "\n";
        result += "    {$PRODOS_AUX_TYPE}: $" + int_to_hex(entry->aux_type) + "\n";
        result += "    {$PRODOS_ACCESS}: $" + int_to_hex(entry->access) + "\n";
        result += "    {$PRODOS_CREATED}: " + date_time(entry->creation_date, entry->creation_time) + "\n";
        result += "    {$PRODOS_MODIFIED}: " + date_time(entry->last_mod_date, entry->last_mod_time) + "\n";

        if (st == PRODOS_ST_SUBDIR) return result;

        Fork data_fork, resource_fork;
        read_fork(*entry, false, data_fork);
        read_fork(*entry, true, resource_fork);

        if (st == PRODOS_ST_EXTENDED) {
            result += "    {$PRODOS_DATA_FORK}: " + std::to_string(data_fork.eof) + " {$BYTES}\n";
            result += "    {$PRODOS_RESOURCE_FORK}: " + std::to_string(resource_fork.eof) + " {$BYTES}\n";
        }

        std::vector<unsigned> blocks;
        if (file_blocks(data_fork, blocks)) {
            result += "\n{$PRODOS_FILE_BLOCKS}:\n    ";
            const size_t used = (fd.size + PRODOS_BLOCK_SIZE - 1) / PRODOS_BLOCK_SIZE;
            size_t shown = 0;
            for (size_t i = 0; i < blocks.size() && i < used; i++) {
                result += std::to_string(blocks[i]) + " ";
                if (++shown % 16 == 0) result += "\n    ";
            }
            result += "\n";
        }

        return result;
    }

    std::vector<std::string> fsProDOS::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    SectorTypeMap fsProDOS::get_sector_type_map()
    {
        SectorTypeMap result;
        if (!is_open) return result;

        auto mark_block = [&](unsigned block, SectorType type, bool overwrite_any) {
            if (block >= total_blocks) return;
            for (unsigned i = 0; i < sectors_per_block; i++) {
                unsigned h, t, s;
                if (!block_to_hts(block, i, h, t, s)) continue;
                const std::array<unsigned, 3> key{h, t, s};
                const auto it = result.find(key);
                if (it == result.end() || overwrite_any) result[key] = type;
            }
        };

        // Boot blocks
        mark_block(0, SectorType::System, false);
        mark_block(1, SectorType::System, false);

        // The free space bitmap
        const unsigned bitmap_blocks = (total_blocks + PRODOS_BLOCK_SIZE * 8 - 1) / (PRODOS_BLOCK_SIZE * 8);
        for (unsigned i = 0; i < bitmap_blocks; i++)
            mark_block(VH.bit_map_pointer + i, SectorType::System, false);

        // Every directory reachable from the volume one, and the files in it
        std::vector<unsigned> dir_queue;
        std::set<unsigned> visited;
        dir_queue.push_back(PRODOS_ROOT_BLOCK);
        visited.insert(PRODOS_ROOT_BLOCK);

        while (!dir_queue.empty()) {
            const unsigned key_block = dir_queue.back();
            dir_queue.pop_back();

            std::vector<BYTES> entries;
            std::vector<unsigned> entry_blocks;
            if (!read_directory(key_block, entries, entry_blocks)) continue;

            for (unsigned block : entry_blocks) mark_block(block, SectorType::Catalog, true);

            for (const auto & raw : entries) {
                const uint8_t st = storage_type(raw.data());
                if (st == PRODOS_ST_DELETED || st == PRODOS_ST_VOLUME_HDR || st == PRODOS_ST_SUBDIR_HDR) continue;

                const auto * entry = reinterpret_cast<const ProDOS_File_Entry *>(raw.data());

                if (st == PRODOS_ST_SUBDIR) {
                    if (block_is_sane(entry->key_pointer, total_blocks) && visited.insert(entry->key_pointer).second)
                        dir_queue.push_back(entry->key_pointer);
                    continue;
                }

                // Index blocks and the key block of an extended file belong to the file
                // as much as its data does
                if (st == PRODOS_ST_SAPLING || st == PRODOS_ST_TREE || st == PRODOS_ST_EXTENDED)
                    mark_block(entry->key_pointer, SectorType::File, false);

                // Both forks of an extended file occupy the disk
                for (int resource = 0; resource < 2; resource++) {
                    Fork fork;
                    if (!read_fork(*entry, resource != 0, fork)) continue;
                    if (!fork.present) continue;

                    if (st == PRODOS_ST_EXTENDED &&
                        (fork.storage_type == PRODOS_ST_SAPLING || fork.storage_type == PRODOS_ST_TREE))
                        mark_block(fork.key_pointer, SectorType::File, false);

                    std::vector<unsigned> blocks;
                    if (!file_blocks(fork, blocks)) continue;
                    for (unsigned block : blocks)
                        if (block != 0) mark_block(block, SectorType::File, false);
                }
            }
        }

        return result;
    }

}
