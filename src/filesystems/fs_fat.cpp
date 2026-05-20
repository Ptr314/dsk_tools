// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: A class and the definitions for the PC FAT filesystem

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <set>
#include <fstream>

#include "dsk_tools/dsk_tools.h"
#include "utils.h"
#include "fs_fat.h"

namespace dsk_tools {

    namespace {
        // Checksum of the 11-byte short name; used to bind LFN entries to their owner.
        uint8_t lfn_checksum(const uint8_t name[11])
        {
            uint8_t sum = 0;
            for (int i = 0; i < 11; i++)
                sum = static_cast<uint8_t>(((sum & 1) ? 0x80 : 0) + (sum >> 1) + name[i]);
            return sum;
        }

        // Pull all 13 UTF-16LE code units out of an LFN slot in their natural order.
        void lfn_extract_chars(const FAT_LFN_ENTRY & lfn, uint16_t out[13])
        {
            for (int i = 0; i < 5; i++)
                out[i]      = static_cast<uint16_t>(lfn.name1[i*2]) | (static_cast<uint16_t>(lfn.name1[i*2 + 1]) << 8);
            for (int i = 0; i < 6; i++)
                out[5 + i]  = static_cast<uint16_t>(lfn.name2[i*2]) | (static_cast<uint16_t>(lfn.name2[i*2 + 1]) << 8);
            for (int i = 0; i < 2; i++)
                out[11 + i] = static_cast<uint16_t>(lfn.name3[i*2]) | (static_cast<uint16_t>(lfn.name3[i*2 + 1]) << 8);
        }

        std::string utf16le_to_utf8(const std::vector<uint16_t> & u16)
        {
            std::string out;
            out.reserve(u16.size());
            for (size_t i = 0; i < u16.size(); i++) {
                uint32_t cp = u16[i];
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < u16.size()) {
                    const uint16_t lo = u16[i + 1];
                    if (lo >= 0xDC00 && lo <= 0xDFFF) {
                        cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                        i++;
                    }
                }
                if (cp < 0x80) {
                    out += static_cast<char>(cp);
                } else if (cp < 0x800) {
                    out += static_cast<char>(0xC0 | (cp >> 6));
                    out += static_cast<char>(0x80 | (cp & 0x3F));
                } else if (cp < 0x10000) {
                    out += static_cast<char>(0xE0 | (cp >> 12));
                    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (cp & 0x3F));
                } else {
                    out += static_cast<char>(0xF0 | (cp >> 18));
                    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
            return out;
        }
    }

    fsFAT::fsFAT(diskImage * image):
        fileSystem(image)
    {}

    FSCaps fsFAT::get_caps()
    {
        return FSCaps::Protect | FSCaps::Dirs | FSCaps::Export | FSCaps::Types;
    }

    uint8_t * fsFAT::read_lba(unsigned lba) const
    {
        const unsigned spt   = image->get_sectors();
        const unsigned heads = image->get_heads();
        if (spt == 0) return nullptr;

        unsigned head, track;
        if (heads <= 1) {
            head  = 0;
            track = lba / spt;
        } else {
            // PC floppy layout: sides are interleaved per track
            head  = (lba / spt) % heads;
            track = (lba / spt) / heads;
        }
        const unsigned sector = lba % spt;
        return image->get_sector_data(head, track, sector);
    }

    unsigned fsFAT::read_fat_entry(unsigned cluster) const
    {
        const unsigned bps = BPB.bytesPerSector;
        if (bps == 0) return 0x0FFFFFFF;

        if (fat_type == FATType::FAT12) {
            const unsigned byte_offset = cluster + (cluster >> 1);   // cluster * 3 / 2
            const unsigned sec    = byte_offset / bps;
            const unsigned in_sec = byte_offset % bps;

            uint8_t * p0 = read_lba(fat_start + sec);
            if (!p0) return 0x0FFFFFFF;

            uint8_t b0 = p0[in_sec];
            uint8_t b1;
            if (in_sec + 1 < bps) {
                b1 = p0[in_sec + 1];
            } else {
                uint8_t * p1 = read_lba(fat_start + sec + 1);
                if (!p1) return 0x0FFFFFFF;
                b1 = p1[0];
            }
            const uint16_t v = static_cast<uint16_t>(b0) | (static_cast<uint16_t>(b1) << 8);
            return (cluster & 1) ? (v >> 4) : (v & 0x0FFF);
        }

        if (fat_type == FATType::FAT16) {
            const unsigned byte_offset = cluster * 2;
            const unsigned sec    = byte_offset / bps;
            const unsigned in_sec = byte_offset % bps;

            uint8_t * p0 = read_lba(fat_start + sec);
            if (!p0) return 0x0FFFFFFF;

            const uint8_t b0 = p0[in_sec];
            uint8_t b1;
            if (in_sec + 1 < bps) {
                b1 = p0[in_sec + 1];
            } else {
                uint8_t * p1 = read_lba(fat_start + sec + 1);
                if (!p1) return 0x0FFFFFFF;
                b1 = p1[0];
            }
            return static_cast<unsigned>(b0) | (static_cast<unsigned>(b1) << 8);
        }

        // FAT32 not supported yet
        return 0x0FFFFFFF;
    }

    bool fsFAT::is_eoc(unsigned fat_entry) const
    {
        switch (fat_type) {
            case FATType::FAT12: return fat_entry >= 0x0FF8;
            case FATType::FAT16: return fat_entry >= 0xFFF8;
            case FATType::FAT32: return fat_entry >= 0x0FFFFFF8;
        }
        return true;
    }

    Result fsFAT::read_directory(uint32_t cluster, BYTES & out) const
    {
        const unsigned bps = BPB.bytesPerSector;
        const unsigned spc = BPB.sectorsPerCluster;
        out.clear();

        if (cluster == 0) {
            // Root directory on FAT12/16: fixed sectors at root_dir_start
            for (unsigned i = 0; i < root_dir_sectors; i++) {
                uint8_t * p = read_lba(root_dir_start + i);
                if (!p) return Result::error(ErrorCode::ReadError);
                out.insert(out.end(), p, p + bps);
            }
            return Result::ok();
        }

        // Subdirectory: follow cluster chain
        uint32_t c = cluster;
        unsigned guard = 0;
        const unsigned guard_max = (total_clusters > 0) ? total_clusters + 2 : 1u << 20;
        while (!is_eoc(c)) {
            if (c < 2) return Result::error(ErrorCode::ReadError);
            const unsigned first_sec = data_start + (c - 2) * spc;
            for (unsigned s = 0; s < spc; s++) {
                uint8_t * p = read_lba(first_sec + s);
                if (!p) return Result::error(ErrorCode::ReadError);
                out.insert(out.end(), p, p + bps);
            }
            c = read_fat_entry(c);
            if (++guard > guard_max) return Result::error(ErrorCode::ReadError);
        }
        return Result::ok();
    }

    std::string fsFAT::make_file_name(const FAT_DIR_ENTRY & de)
    {
        std::string base(reinterpret_cast<const char *>(&de.name[0]), 8);
        std::string ext (reinterpret_cast<const char *>(&de.name[8]), 3);

        // First byte 0x05 actually represents 0xE5 (Kanji workaround)
        if (!base.empty() && static_cast<uint8_t>(base[0]) == 0x05)
            base[0] = static_cast<char>(0xE5);

        base = trim(base);
        ext  = trim(ext);
        return ext.empty() ? base : (base + "." + ext);
    }

    Result fsFAT::open()
    {
        if (!image->get_loaded()) return Result::error(ErrorCode::OpenNotLoaded);

        uint8_t * boot = read_lba(0);
        if (!boot) return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "Cannot read FAT boot sector"));

        std::memcpy(&BPB, boot, sizeof(BPB));

        // Sanity checks
        if (BPB.bytesPerSector == 0 || (BPB.bytesPerSector & (BPB.bytesPerSector - 1)) != 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: invalid bytes per sector"));
        if (BPB.sectorsPerCluster == 0 || (BPB.sectorsPerCluster & (BPB.sectorsPerCluster - 1)) != 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: invalid sectors per cluster"));
        if (BPB.numFATs == 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: invalid FAT count"));
        if (BPB.reservedSectors == 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: invalid reserved sector count"));
        if (BPB.bytesPerSector != image->get_sector_size())
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: sector size mismatch"));

        const unsigned total_sectors = (BPB.totalSectors16 != 0) ? BPB.totalSectors16 : BPB.totalSectors32;
        if (total_sectors == 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: zero total sectors"));

        const unsigned sectors_per_fat = BPB.sectorsPerFAT16; // FAT32 stored elsewhere; not supported yet
        if (sectors_per_fat == 0)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT32 is not supported"));

        const unsigned root_entry_bytes = static_cast<unsigned>(BPB.rootEntries) * sizeof(FAT_DIR_ENTRY);
        root_dir_sectors = (root_entry_bytes + BPB.bytesPerSector - 1) / BPB.bytesPerSector;

        fat_start      = BPB.reservedSectors;
        root_dir_start = fat_start + static_cast<unsigned>(BPB.numFATs) * sectors_per_fat;
        data_start     = root_dir_start + root_dir_sectors;

        if (data_start >= total_sectors)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT: data region beyond disk"));

        const unsigned data_sectors = total_sectors - data_start;
        total_clusters = data_sectors / BPB.sectorsPerCluster;

        // Microsoft's "Hardware White Paper" rules
        if (total_clusters < 4085) fat_type = FATType::FAT12;
        else if (total_clusters < 65525) fat_type = FATType::FAT16;
        else fat_type = FATType::FAT32;

        if (fat_type == FATType::FAT32)
            return Result::error(ErrorCode::OpenBadFormat, QT_TRANSLATE_NOOP("errors", "FAT32 is not supported"));

        current_path.clear();
        current_path.push_back(0); // 0 = root for FAT12/16

        volume_id = BPB.mediaDescriptor;
        is_open = true;
        return Result::ok();
    }

    void fsFAT::cd_up()
    {
        if (current_path.size() > 1)
            current_path.pop_back();
    }

    void fsFAT::cd(const UniversalFile & dir, bool & updir)
    {
        if (dir.name == "..") {
            cd_up();
            updir = true;
        } else {
            if (dir.metadata.size() >= sizeof(FAT_DIR_ENTRY)) {
                const auto * de = reinterpret_cast<const FAT_DIR_ENTRY *>(dir.metadata.data());
                const uint32_t cluster = (static_cast<uint32_t>(de->firstClusterHi) << 16) | de->firstClusterLo;
                current_path.push_back(cluster);
            }
            updir = false;
        }
    }

    std::string fsFAT::file_info(const UniversalFile & fd) {
        std::string result;
        //TODO: Impelement
        return result;
    }

    std::vector<std::string> fsFAT::get_save_file_formats()
    {
        return {"FILE_BINARY"};
    }

    std::vector<std::string> fsFAT::get_add_file_formats()
    {
        return {"FILE_BINARY"};
    }

    std::string fsFAT::information()
    {
        if (!is_open) return "";

        std::string oem = trim(std::string(reinterpret_cast<const char *>(BPB.OEMName), 8));

        const char * type_name = (fat_type == FATType::FAT12) ? "FAT12"
                               : (fat_type == FATType::FAT16) ? "FAT16"
                                                              : "FAT32";

        const unsigned total_sectors = (BPB.totalSectors16 != 0) ? BPB.totalSectors16 : BPB.totalSectors32;
        const unsigned cluster_bytes = static_cast<unsigned>(BPB.bytesPerSector) * BPB.sectorsPerCluster;

        unsigned free_clusters = 0;
        unsigned bad_clusters  = 0;
        unsigned used_clusters = 0;
        const unsigned bad_marker = (fat_type == FATType::FAT12) ? 0x0FF7u : 0xFFF7u;
        for (unsigned c = 2; c < total_clusters + 2; c++) {
            const unsigned v = read_fat_entry(c);
            if (v == 0) free_clusters++;
            else if (v == bad_marker) bad_clusters++;
            else used_clusters++;
        }

        std::string result;
        result += "{$FAT_FS_TYPE}: " + std::string(type_name) + "\n";
        result += "{$FAT_OEM_NAME}: " + oem + " [" + toHexList(BPB.OEMName, 8, "$") + "]\n";
        result += "{$FAT_MEDIA_DESCRIPTOR}: $" + int_to_hex(BPB.mediaDescriptor) + "\n";
        result += "\n";

        result += "{$FAT_BPB_GEOMETRY}:\n";
        result += "    {$FAT_BYTES_PER_SECTOR}: " + std::to_string(BPB.bytesPerSector) + "\n";
        result += "    {$FAT_SECTORS_PER_CLUSTER}: " + std::to_string(BPB.sectorsPerCluster) + "\n";
        result += "    {$FAT_CLUSTER_SIZE}: " + std::to_string(cluster_bytes) + " {$BYTES}\n";
        result += "    {$FAT_SECTORS_PER_TRACK}: " + std::to_string(BPB.sectorsPerTrack) + "\n";
        result += "    {$FAT_NUM_HEADS}: " + std::to_string(BPB.numHeads) + "\n";
        result += "    {$FAT_TOTAL_SECTORS}: " + std::to_string(total_sectors)
               + " (16-bit: " + std::to_string(BPB.totalSectors16)
               + ", 32-bit: " + std::to_string(BPB.totalSectors32) + ")\n";
        result += "    {$FAT_HIDDEN_SECTORS}: " + std::to_string(BPB.hiddenSectors) + "\n";
        result += "\n";

        result += "{$FAT_LAYOUT}:\n";
        result += "    {$FAT_RESERVED_SECTORS}: " + std::to_string(BPB.reservedSectors) + "\n";
        result += "    {$FAT_NUM_FATS}: " + std::to_string(BPB.numFATs) + "\n";
        result += "    {$FAT_SECTORS_PER_FAT}: " + std::to_string(BPB.sectorsPerFAT16) + "\n";
        result += "    {$FAT_ROOT_ENTRIES}: " + std::to_string(BPB.rootEntries) + "\n";
        result += "    {$FAT_ROOT_DIR_SECTORS}: " + std::to_string(root_dir_sectors) + "\n";
        result += "\n";

        result += "{$FAT_REGIONS_LBA}:\n";
        result += "    {$FAT_FAT_START}: " + std::to_string(fat_start) + "\n";
        result += "    {$FAT_ROOT_DIR_START}: " + std::to_string(root_dir_start) + "\n";
        result += "    {$FAT_DATA_START}: " + std::to_string(data_start) + "\n";
        result += "\n";

        result += "{$FAT_CLUSTERS}:\n";
        result += "    {$FAT_TOTAL_CLUSTERS}: " + std::to_string(total_clusters) + "\n";
        result += "    {$FAT_USED_CLUSTERS}: " + std::to_string(used_clusters) + "\n";
        result += "    {$FAT_FREE_CLUSTERS}: " + std::to_string(free_clusters) + "\n";
        result += "    {$FAT_BAD_CLUSTERS}: " + std::to_string(bad_clusters) + "\n";
        result += "    {$FREE_BYTES}: " + std::to_string(static_cast<uint64_t>(free_clusters) * cluster_bytes) + "\n";

        return result;
    }

    bool fsFAT::is_root()
    {
        return current_path.size() <= 1;
    }

    Result fsFAT::get_file(const UniversalFile & uf, const std::string & format, BYTES & data) const
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);
        if (uf.metadata.size() < sizeof(FAT_DIR_ENTRY))
            return Result::error(ErrorCode::FileIncorrectFS);

        data.clear();

        const auto * de = reinterpret_cast<const FAT_DIR_ENTRY *>(uf.metadata.data());
        const uint32_t first_cluster = (static_cast<uint32_t>(de->firstClusterHi) << 16) | de->firstClusterLo;
        const uint32_t file_size = de->fileSize;

        if (file_size == 0 || first_cluster == 0) return Result::ok();

        const unsigned bps = BPB.bytesPerSector;
        const unsigned spc = BPB.sectorsPerCluster;

        data.reserve(file_size);

        uint32_t c = first_cluster;
        unsigned guard = 0;
        const unsigned guard_max = (total_clusters > 0) ? total_clusters + 2 : 1u << 20;

        while (!is_eoc(c)) {
            if (c < 2) return Result::error(ErrorCode::ReadError);

            const unsigned first_sec = data_start + (c - 2) * spc;
            for (unsigned s = 0; s < spc; s++) {
                uint8_t * p = read_lba(first_sec + s);
                if (!p) return Result::error(ErrorCode::ReadError);
                data.insert(data.end(), p, p + bps);
                if (data.size() >= file_size) {
                    data.resize(file_size);
                    return Result::ok();
                }
            }

            c = read_fat_entry(c);
            if (++guard > guard_max) return Result::error(ErrorCode::ReadError);
        }

        if (data.size() > file_size) data.resize(file_size);
        return Result::ok();
    }

    Result fsFAT::dir(std::vector<UniversalFile> & files, bool show_deleted)
    {
        if (!is_open) return Result::error(ErrorCode::OpenNotLoaded);

        files.clear();

        const uint32_t cur_cluster = current_path.empty() ? 0 : current_path.back();

        if (current_path.size() > 1) {
            UniversalFile updir;
            updir.name = "..";
            updir.is_dir = true;
            updir.is_deleted = false;
            files.push_back(updir);
        }

        BYTES buffer;
        auto res = read_directory(cur_cluster, buffer);
        if (!res) return res;

        const std::set<std::string> txts = {".txt", ".bat", ".ini", ".doc", ".asm", ".c", ".h", ".cpp", ".pas", ".me"};

        // LFN accumulator: VFAT stores long names in 1..N preceding 0x0F-attr slots,
        // highest ordinal first (with the 0x40 "last" bit set), bound to the short
        // entry via a checksum of the 11-byte short name.
        std::vector<std::vector<uint16_t>> lfn_parts;
        uint8_t lfn_checksum_expected = 0;
        bool lfn_valid = false;

        const size_t entries = buffer.size() / sizeof(FAT_DIR_ENTRY);
        for (size_t i = 0; i < entries; i++) {
            const auto * de = reinterpret_cast<const FAT_DIR_ENTRY *>(buffer.data() + i * sizeof(FAT_DIR_ENTRY));

            const uint8_t first = de->name[0];

            if (first == 0x00) break;                       // no further entries past this point

            // LFN sub-entry — accumulate, then continue to its companion short entry.
            if (de->attr == FAT_ATTR_LONG_NAME) {
                if (first == 0xE5) {
                    // Deleted LFN sub-entry orphans the chain — drop it.
                    lfn_parts.clear();
                    lfn_valid = false;
                    continue;
                }
                const auto * lfn = reinterpret_cast<const FAT_LFN_ENTRY *>(de);
                const uint8_t ord = lfn->ord;
                const uint8_t idx = static_cast<uint8_t>((ord & 0x1F) - 1);

                uint16_t chars[13];
                lfn_extract_chars(*lfn, chars);

                if (ord & 0x40) {
                    // First sub-entry we encounter is the highest-ordinal one — size to fit.
                    lfn_parts.assign(static_cast<size_t>(idx) + 1, std::vector<uint16_t>());
                    lfn_checksum_expected = lfn->checksum;
                    lfn_valid = true;
                }
                if (!lfn_valid || idx >= lfn_parts.size() || lfn->checksum != lfn_checksum_expected) {
                    lfn_parts.clear();
                    lfn_valid = false;
                    continue;
                }
                lfn_parts[idx].assign(chars, chars + 13);
                continue;
            }

            if (de->attr & FAT_ATTR_VOLUME_ID) {            // volume label entry
                lfn_parts.clear();
                lfn_valid = false;
                continue;
            }

            const bool is_deleted = (first == 0xE5);
            if (is_deleted && !show_deleted) {
                lfn_parts.clear();
                lfn_valid = false;
                continue;
            }

            UniversalFile f;
            f.fs = get_fs();

            std::string short_name;
            if (is_deleted) {
                // First byte of a deleted entry is overwritten with 0xE5;
                // surface a placeholder so the rest of the 8.3 name stays readable.
                FAT_DIR_ENTRY shown = *de;
                shown.name[0] = '?';
                short_name = make_file_name(shown);
            } else {
                short_name = make_file_name(*de);
            }

            // Use the accumulated long name if the chain is complete and the
            // checksum matches the short name we're sitting on.
            std::string long_name;
            if (!is_deleted && lfn_valid && lfn_checksum(de->name) == lfn_checksum_expected) {
                std::vector<uint16_t> all_chars;
                bool complete = true;
                bool terminator_seen = false;
                for (const auto & p : lfn_parts) {
                    if (p.empty()) { complete = false; break; }
                    if (terminator_seen) continue;
                    for (uint16_t c : p) {
                        if (c == 0x0000 || c == 0xFFFF) { terminator_seen = true; break; }
                        all_chars.push_back(c);
                    }
                }
                if (complete) long_name = utf16le_to_utf8(all_chars);
            }
            lfn_parts.clear();
            lfn_valid = false;

            f.name = long_name.empty() ? short_name : long_name;

            // Drop on-disk "." and ".." rows — the parent ".." is synthesized above
            // from current_path. Relies on make_file_name preserving the dots.
            if (!is_deleted && (f.name == "." || f.name == "..")) continue;

            f.is_dir       = (de->attr & FAT_ATTR_DIRECTORY) != 0;
            f.is_deleted   = is_deleted;
            f.is_protected = (de->attr & FAT_ATTR_READ_ONLY) != 0;
            f.attributes   = de->attr;
            f.size         = de->fileSize;

            f.type_preferred = PreferredType::Binary;
            const std::string ext = get_file_ext(f.name);
            if (txts.find(to_lower(ext)) != txts.end()) f.type_preferred = PreferredType::Text;

            std::string label;
            label += (de->attr & FAT_ATTR_READ_ONLY) ? "R" : "";
            label += (de->attr & FAT_ATTR_HIDDEN)    ? "H" : "";
            label += (de->attr & FAT_ATTR_SYSTEM)    ? "S" : "";
            label += (de->attr & FAT_ATTR_ARCHIVE)   ? "A" : "";
            f.type_label = label;

            f.original_name.assign(de->name, de->name + 11);

            f.metadata.resize(sizeof(FAT_DIR_ENTRY));
            std::memcpy(f.metadata.data(), de, sizeof(FAT_DIR_ENTRY));

            files.push_back(f);
        }

        return Result::ok();
    }

}
