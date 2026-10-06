// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Main source file

#include <iostream>
#include <fstream>
#include <algorithm>

#include "dsk_tools/dsk_tools.h"

#include "fs_iskra226.h"
#include "host_helpers.h"

namespace dsk_tools {

    std::unique_ptr<Loader> create_loader(const std::string& file_name, const std::string& format_id, const std::string& type_id)
    {
        if (format_id == "FILE_RAW_MSB") return dsk_tools::make_unique<LoaderRAW>(file_name, format_id, type_id);
        if (format_id == "FILE_AIM")     return dsk_tools::make_unique<LoaderAIM>(file_name, format_id, type_id);
        if (format_id == "FILE_MFM_NIC") return dsk_tools::make_unique<LoaderNIC>(file_name, format_id, type_id);
        if (format_id == "FILE_MFM_NIB") return dsk_tools::make_unique<LoaderNIB>(file_name, format_id, type_id);
        if (format_id == "FILE_HXC_MFM") return dsk_tools::make_unique<LoaderHXC_MFM>(file_name, format_id, type_id);
        if (format_id == "FILE_HXC_HFE") return dsk_tools::make_unique<LoaderHXC_HFE>(file_name, format_id, type_id);
        if (format_id == "FILE_FIL")     return dsk_tools::make_unique<LoaderFIL>(file_name, format_id, type_id);
        if (format_id == "FILE_IMD")     return dsk_tools::make_unique<LoaderIMD>(file_name, format_id, type_id);
        return nullptr;
    }

    std::unique_ptr<diskImage> prepare_image(const std::string &file_name, const std::string &format_id, const std::string &type_id, const DiskDefs & diskdefs)
    {
        std::unique_ptr<Loader> loader = create_loader(file_name, format_id, type_id);
        if (!loader) return nullptr;

        if (type_id == "TYPE_AGAT_140")   return dsk_tools::make_unique<imageAgat140>(std::move(loader));
        if (type_id == "TYPE_AGAT_840")   return dsk_tools::make_unique<imageAgat840>(std::move(loader));
        if (type_id == "TYPE_AGAT_880")   return dsk_tools::make_unique<imageAgat880>(std::move(loader));
        if (type_id == "TYPE_FIL")        return dsk_tools::make_unique<imageFIL>(std::move(loader));
        if (type_id.rfind("TYPE_CPM:", 0)==0 || type_id.rfind("TYPE_FAT:", 0)==0 || type_id.rfind("TYPE_OTHER:", 0)==0 || type_id.rfind("TYPE_RT11:", 0)==0) {
            const std::string diskdef_id = to_lower(type_id.substr(type_id.find(':') + 1));
            const auto it = diskdefs.find(diskdef_id);
            if (it == diskdefs.end()) return nullptr;
            const DiskDef &diskdef = it->second;

            unsigned heads = 0;
            if (!get_map_value(diskdef.int_params, std::string("heads"), heads, 2, false)) return nullptr;
            unsigned tracks = 0;
            if (!get_map_value(diskdef.int_params, std::string("tracks"), tracks, 0, true)) return nullptr;
            unsigned sectrk = 0;
            if (!get_map_value(diskdef.int_params, std::string("sectrk"), sectrk, 0, true)) return nullptr;
            unsigned seclen = 0;
            if (!get_map_value(diskdef.int_params, std::string("seclen"), seclen, 0, true)) return nullptr;
            // Data rate and rpm matter only to the formats that keep whole tracks
            unsigned bitrate = 0;
            get_map_value(diskdef.int_params, std::string("bitrate"), bitrate, 0, false);
            unsigned rpm = 0;
            get_map_value(diskdef.int_params, std::string("rpm"), rpm, 0, false);
            const unsigned track_enc = UNKNOWN_ENCODING;
            const unsigned iface = GENERIC_SHUGGART_DD_FLOPPYMODE;
            const unsigned sector_base = 1;

            bool side_ilvd;
            std::string sides = "";
            if (!get_map_value(diskdef.str_params, std::string("sides"), sides, std::string("int"), false)) return nullptr;
            if (sides == "int") side_ilvd = true;
            else if (sides == "seq") side_ilvd = false;
            else return nullptr;

            std::vector<unsigned> skewtab = diskdef.skewtab;

            DiskFormatParams params(
                heads,                    // heads
                tracks,                   // tracks
                sectrk,                   // sectors
                seclen,                   // sector size
                bitrate,                  // bitrate
                rpm,                      // rpm
                track_enc,                // track encoding
                iface,                    // floppy interface mode
                sector_base,              // sector base
                side_ilvd,                // sides interleaved
                skewtab                   // sector translation
            );

            // How the tracks are laid out when the image keeps them whole (HFE)
            std::string layout;
            get_map_value(diskdef.str_params, std::string("layout"), layout, std::string(), false);
            params.layout = track_layout_by_name(layout);
            if (params.layout != TrackLayout::None) {
                track_layout_defaults(params);
                const auto gap = [&](const char * key, int & value) {
                    unsigned v = 0;
                    if (get_map_value(diskdef.int_params, std::string(key), v, 0, true)) value = static_cast<int>(v);
                };
                gap("gap4a", params.gaps.gap4a);
                gap("gap1", params.gaps.gap1);
                gap("gap2", params.gaps.gap2);
                gap("gap3", params.gaps.gap3);
                unsigned index_mark = 1;
                get_map_value(diskdef.int_params, std::string("indexmark"), index_mark, 1, false);
                params.gaps.index_mark = index_mark != 0;
            }

            return dsk_tools::make_unique<diskImage>(std::move(loader), params);
        }
        return nullptr;
    }

    std::unique_ptr<fileSystem> prepare_filesystem(diskImage * image, const std::string &filesystem_id, const DiskDefs & diskdefs)
    {
        if (filesystem_id == "FILESYSTEM_DOS33") {
            return dsk_tools::make_unique<fsDOS33>(image);
        }
        if (filesystem_id == "FILESYSTEM_SPRITE_OS") {
            return dsk_tools::make_unique<fsSpriteOS>(image);
        }
        if (filesystem_id == "FILESYSTEM_PRODOS") {
            return dsk_tools::make_unique<fsProDOS>(image);
        }
        if (filesystem_id == "FILESYSTEM_ONIX") {
            return dsk_tools::make_unique<fsOnix>(image);
        }
        if (filesystem_id == "FILESYSTEM_CPM_DOS" || filesystem_id == "FILESYSTEM_CPM_PRODOS"|| filesystem_id == "FILESYSTEM_CPM_RAW") {
            return dsk_tools::make_unique<fsCPM>(image, filesystem_id, diskdefs);
        }
        if (filesystem_id == "FILESYSTEM_FIL") {
            return dsk_tools::make_unique<fsFIL>(image);
        }
        if (filesystem_id == "FILESYSTEM_FAT") {
            return dsk_tools::make_unique<fsFAT>(image);
        }
        if (filesystem_id == "FILESYSTEM_ISKRA-226") {
            return dsk_tools::make_unique<fsIskra226>(image);
        }
        if (filesystem_id == "FILESYSTEM_RT11") {
            return dsk_tools::make_unique<fsRT11>(image);
        }
        return nullptr;
    }


    // A ProDOS volume keeps its directory in block 2, so an image whose sectors are
    // stored in ProDOS block order has the volume header at a fixed offset
    static bool is_prodos_volume(UTF8_ifstream & file, const unsigned disk_blocks)
    {
        BYTES block(PRODOS_BLOCK_SIZE);
        file.seekg(PRODOS_ROOT_BLOCK * PRODOS_BLOCK_SIZE, std::ios::beg);
        file.read(reinterpret_cast<char*>(block.data()), block.size());
        if (!file.good()) return false;
        return fsProDOS::volume_header_is_valid(block, disk_blocks);
    }

    // An Onix volume keeps its allocation table in the blocks right after the boot sector,
    // so an image stored in plain sector order carries the signature at a fixed offset
    static bool is_onix_volume(UTF8_ifstream & file, const unsigned disk_blocks)
    {
        BYTES fat_area(ONIX_FAT_BLOCKS * ONIX_BLOCK_SIZE);
        file.seekg(ONIX_FAT_BLOCK * ONIX_BLOCK_SIZE, std::ios::beg);
        file.read(reinterpret_cast<char*>(fat_area.data()), fat_area.size());
        if (!file.good()) return false;

        unsigned root = 0;
        if (!fsOnix::fat_is_valid(fat_area, disk_blocks, root)) return false;

        BYTES root_data(ONIX_BLOCK_SIZE);
        file.seekg(root * ONIX_BLOCK_SIZE, std::ios::beg);
        file.read(reinterpret_cast<char*>(root_data.data()), root_data.size());
        if (!file.good()) return false;

        return fsOnix::dir_block_is_valid(root_data);
    }

    // An RT-11 volume has its home block in block 1 and the first directory segment
    // where the home block says. A DX image keeps its sectors in physical order, so a
    // block is gathered from four interleaved sectors; any other image is linear.
    // read(offset, out, size) fetches bytes of the image
    template <typename R>
    static bool is_rt11_volume(R read, const unsigned disk_blocks, const bool dx)
    {
        auto read_block = [&](unsigned block, BYTES & out) -> bool {
            out.assign(RT11_BLOCK_SIZE, 0);
            if (dx) {
                const unsigned per_block = RT11_BLOCK_SIZE / RT11_DX_SECTOR_SIZE;
                for (unsigned i = 0; i < per_block; i++) {
                    unsigned track, sector;
                    fsRT11::dx_sector(block * per_block + i, track, sector);
                    if (!read((track * RT11_DX_SECTORS + sector) * RT11_DX_SECTOR_SIZE, &out[i * RT11_DX_SECTOR_SIZE], RT11_DX_SECTOR_SIZE))
                        return false;
                }
                return true;
            }
            return read(block * RT11_BLOCK_SIZE, out.data(), out.size());
        };

        BYTES home, segment;
        if (!read_block(RT11_HOME_BLOCK, home)) return false;
        const unsigned dir_block = fsRT11::directory_block(home, disk_blocks);
        if (!read_block(dir_block, segment)) return false;
        return fsRT11::segment_is_valid(segment, dir_block, disk_blocks);
    }

    static bool is_rt11_volume(UTF8_ifstream & file, const unsigned disk_blocks, const bool dx)
    {
        return is_rt11_volume([&](size_t offset, uint8_t * out, size_t size) {
            file.seekg(offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(out), size);
            return file.good();
        }, disk_blocks, dx);
    }

    static bool is_rt11_volume(const BYTES & image, const unsigned disk_blocks, const bool dx)
    {
        return is_rt11_volume([&](size_t offset, uint8_t * out, size_t size) {
            if (offset + size > image.size()) return false;
            memcpy(out, image.data() + offset, size);
            return true;
        }, disk_blocks, dx);
    }

    // A FAT boot sector: a BIOS parameter block that makes sense. DOS ends it
    // with 55 AA or names itself in the OEM field, the Atari ST keeps a serial
    // number there. Neither is certain - ST disks were often formatted on a
    // PC - but the filesystem is the same, only the name of the type differs
    static bool is_fat_boot(const BYTES & image, bool & pc)
    {
        if (image.size() < 512) return false;
        const uint8_t * b = image.data();
        const unsigned bytes_per_sector = b[11] | (b[12] << 8);
        const unsigned per_cluster = b[13];
        bool oem_text = true;
        for (int i = 3; i < 11; i++)
            if (b[i] < 0x20 || b[i] > 0x7E) oem_text = false;
        pc = (b[510] == 0x55 && b[511] == 0xAA) || oem_text;
        return bytes_per_sector == 512 && per_cluster != 0 && (per_cluster & (per_cluster - 1)) == 0
               && b[16] >= 1 && b[16] <= 2 && b[21] >= 0xF0;
    }

    // How much an area looks like a CP/M directory: the number of entries with
    // a user number and a name of printable characters, -1 when an entry is
    // neither that nor empty (E5). A blank area scores 0
    static int cpm_directory_score(const BYTES & image, size_t offset, size_t size)
    {
        if (offset + size > image.size()) return -1;
        int entries = 0;
        for (size_t e = offset; e + 32 <= offset + size; e += 32) {
            const uint8_t user = image[e];
            if (user == 0xE5) continue;
            if (user > 31) return -1;
            for (int i = 1; i <= 11; i++)
                if ((image[e + i] & 0x7F) < 0x20) return -1;
            entries++;
        }
        return entries;
    }

    // Whether the last track of a disk with this many tracks holds sectors
    static bool hfe_track_present(const HfeImage & img, const DiskFormatParams & format, unsigned tracks)
    {
        if (tracks == 0 || static_cast<int>(tracks) > img.tracks) return false;
        BYTES out(static_cast<size_t>(format.sectors) * format.sector_size);
        TrackStatus status;
        return track_from_cells(format, img.cells[static_cast<size_t>(tracks - 1) * img.sides], out.data(), status);
    }

    // The type and the filesystem of a disk in an HFE by the geometry its
    // tracks show and by what the volume itself says
    static bool detect_hfe_tracks(const HfeImage & img, std::string & type_id, std::string & filesystem_id)
    {
        DiskFormatParams format;
        if (!hfe_probe_format(img, format)) return false;

        const unsigned heads = format.heads;
        const unsigned sectors = format.sectors;
        const unsigned size = format.sector_size;
        const bool mfm = format.layout == TrackLayout::IbmMfm;
        const bool fm = format.layout == TrackLayout::IbmFm;
        auto has_tracks = [&](unsigned tracks) { return hfe_track_present(img, format, tracks); };
        // The volume itself, decoded with the geometry of the given track count
        auto image_of = [&](unsigned tracks, BYTES & image) {
            DiskFormatParams f = format;
            f.tracks = tracks;
            hfe_to_flat(img, f, image);
        };

        BYTES image;
        bool pc = false;
        if (format.layout == TrackLayout::DvkMx) {
            type_id = has_tracks(80) ? "TYPE_RT11:MX-440" : "TYPE_RT11:MX-220";
            filesystem_id = "FILESYSTEM_RT11";
        } else
        if (fm && heads == 1 && sectors == 26 && size == 128) {
            image_of(77, image);
            if (is_rt11_volume(image, RT11_DX_BLOCKS, true)) {
                type_id = "TYPE_RT11:DX";
                filesystem_id = "FILESYSTEM_RT11";
            } else {
                // The 8" GMD-7012 of the Irisha and the DVK DX are the same disk
                type_id = "TYPE_CPM:GMD-7012";
                filesystem_id = "FILESYSTEM_CPM_RAW";
            }
        } else
        if (mfm && size == 512 && sectors == 10) {
            const unsigned tracks = has_tracks(82) ? 82 : 80;
            image_of(tracks, image);
            if (heads == 1) {
                type_id = "TYPE_RT11:MZ-400";
                filesystem_id = "FILESYSTEM_RT11";
            } else
            if (!is_rt11_volume(image, 1600, false) && is_fat_boot(image, pc)) {
                type_id = (tracks == 82) ? "TYPE_FAT:ST-820" : "TYPE_FAT:ST-800";
                filesystem_id = "FILESYSTEM_FAT";
            } else {
                // DVK MY, UKNC and BK MZ disks share the geometry, as with the raw images
                type_id = "TYPE_RT11:MY-800";
                filesystem_id = "FILESYSTEM_RT11";
            }
        } else
        if (mfm && heads == 2 && size == 512 && (sectors == 9 || sectors == 15 || sectors == 18)) {
            const unsigned tracks = (sectors == 9 && !has_tracks(80)) ? 40 : 80;
            image_of(tracks, image);
            const bool fat = is_fat_boot(image, pc);
            filesystem_id = "FILESYSTEM_FAT";
            if (sectors == 15) {
                type_id = "TYPE_FAT:PC-1200";
            } else
            if (sectors == 18) {
                type_id = (fat && !pc) ? "TYPE_FAT:ST-1440" : "TYPE_FAT:PC-1440";
            } else
            if (tracks == 80) {
                type_id = (fat && !pc) ? "TYPE_FAT:ST-720" : "TYPE_FAT:PC-720";
            } else
            if (fat) {
                type_id = pc ? "TYPE_FAT:PC-360" : "TYPE_FAT:ST-360";
            } else {
                type_id = "TYPE_CPM:IRISHA-360-INT";
                filesystem_id = "FILESYSTEM_CPM_RAW";
            }
        } else
        if (mfm && heads == 2 && size == 1024 && sectors == 5) {
            // Korvet, Orion and Vector differ in the system tracks before the
            // directory: 1, 2 and 4, a track being both sides of a cylinder
            const size_t track = 2 * 5 * 1024;
            const size_t dir = 4096;
            const unsigned tracks = has_tracks(82) ? 82 : 80;
            image_of(tracks, image);
            const int korvet = cpm_directory_score(image, track, dir);
            const int orion = cpm_directory_score(image, 2 * track, dir);
            const int vector = cpm_directory_score(image, 4 * track, dir);
            if (vector > 0 && vector >= korvet && vector >= orion)
                type_id = "TYPE_CPM:VECTOR";
            else
            if (orion > 0 && orion >= korvet)
                type_id = "TYPE_CPM:ORION";
            else
            if (korvet > 0)
                type_id = "TYPE_CPM:KORVET";
            else
                type_id = (tracks == 82) ? "TYPE_CPM:VECTOR" : "TYPE_CPM:KORVET";
            filesystem_id = "FILESYSTEM_CPM_RAW";
        } else
            return false;
        return true;
    }

    Result detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id, bool format_only)
    {
        std::string ext = get_file_ext(file_name);

        std::unique_ptr<UTF8_ifstream> file = nullptr;
        unsigned fsize = 0;

        if (!format_only) {
            file = make_unique<UTF8_ifstream>(file_name, std::ios::binary);

            if (!file->good()) {
                return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Cannot open file"));
            }

            file->seekg (0, std::ios::end);
            fsize = file->tellg();
            file->seekg (0, std::ios::beg);
        }

        // format_if
        if (ext == ".kdi" && (fsize == 819200 || format_only)) {
            format_id = "FILE_RAW_MSB";
            type_id = "TYPE_CPM:KORVET";
            filesystem_id = "FILESYSTEM_CPM_RAW";
            return Result::ok();
        }
        if (ext == ".fdd" && (fsize == 839680 || format_only)) {
            format_id = "FILE_RAW_MSB";
            type_id = "TYPE_CPM:VECTOR";
            filesystem_id = "FILESYSTEM_CPM_RAW";
            return Result::ok();
        }
        if ((ext == ".odi" || ext == ".fdd") && (fsize == 819200 || format_only)) {
            format_id = "FILE_RAW_MSB";
            type_id = "TYPE_CPM:ORION";
            filesystem_id = "FILESYSTEM_CPM_RAW";
            return Result::ok();
        }

        if (ext == ".st") {
            format_id = "FILE_RAW_MSB";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return Result::ok();
            }

            if (fsize == 512*9*40*2) {
                type_id = "TYPE_FAT:ST-360";
            } else
            if (fsize == 512*9*80*2) {
                type_id = "TYPE_FAT:ST-720";
            } else
            if (fsize == 512*10*80*2) {
                type_id = "TYPE_FAT:ST-800";
            } else
            if (fsize == 512*10*82*2) {
                type_id = "TYPE_FAT:ST-820";
            } else
            if (fsize == 512*18*80*2) {
                type_id = "TYPE_FAT:ST-1440";
            } else
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid file size for DSK format"));

            filesystem_id = "FILESYSTEM_FAT";
            return Result::ok();
        }

        if (ext == ".dsk" || ext == ".do" || ext == ".po" || ext == ".cpm" || ext == ".gmd" || ext == ".fdd" || ext == ".img" || ext == ".ima" || ext == ".bkd" || ext == ".rtd") {
            format_id = "FILE_RAW_MSB";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return Result::ok();
            }

            // type_id
            if (ext == ".rtd") {
                // RT-11 disks of the UKNC and BK emulators: a run of 512 byte blocks, as many
                // as the volume holds, sometimes behind a 256 byte header
                const unsigned header = (fsize % RT11_BLOCK_SIZE == 256) ? 256 : 0;
                const unsigned blocks = (fsize - header) / RT11_BLOCK_SIZE;
                if (blocks <= 800)
                    type_id = "TYPE_RT11:MZ-400";
                else
                if (blocks <= 1600)
                    type_id = "TYPE_RT11:MZ-800";
                else
                    return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid file size for DSK format"));
            } else
            if (fsize == 143360 || fsize == 143360+128) {
                type_id = "TYPE_AGAT_140";
            } else
            if (fsize == 860160 || fsize == 860164) {
                type_id = "TYPE_AGAT_840";
            } else
            if (fsize == 901120 || fsize == 901124) {
                // 160 tracks of 11 sectors of 512 bytes, the ProDOS format of the Nippel OS
                type_id = "TYPE_AGAT_880";
            } else
            if (fsize == 1600*PRODOS_BLOCK_SIZE && is_prodos_volume(*file, 1600)) {
                // Apple 3.5" 800 Kb: 1600 ProDOS blocks one after another
                type_id = "TYPE_OTHER:PRODOS-800";
            } else
            // if (fsize == 512*9*40*2) {
            //     type_id = "TYPE_CPM:IRISHA-360-INT";
            // } else
            if (fsize == 1600*RT11_BLOCK_SIZE && is_rt11_volume(*file, 1600, false)) {
                // DVK MY, UKNC and BK MZ disks share the geometry, so either name will do;
                // the MY/MZ choice only matters to the user picking a type by hand.
                // BK disks of ANDOS or MKDOS are the same size and are left to manual choice.
                type_id = (ext == ".bkd") ? "TYPE_RT11:MZ-800" : "TYPE_RT11:MY-800";
            } else
            if (fsize == 128*26*77) {
                // The 8" GMD-7012 of the Irisha and the DVK DX are the same disk
                type_id = is_rt11_volume(*file, RT11_DX_BLOCKS, true) ? "TYPE_RT11:DX" : "TYPE_CPM:GMD-7012";
            } else
            if (fsize == 256*11*40*2) {
                type_id = "TYPE_RT11:MX-220";
            } else
            if (fsize == 256*11*80*2) {
                type_id = "TYPE_RT11:MX-440";
            } else
            if (fsize == 512*10*80) {
                type_id = "TYPE_RT11:MZ-400";
            } else
            if (fsize == 512*9*40*2) {
                type_id = "TYPE_FAT:PC-360";
            } else
            if (fsize == 512*9*80*2) {
                type_id = "TYPE_FAT:PC-720";
            } else
            if (fsize == 512*15*80*2) {
                type_id = "TYPE_FAT:PC-1200";
            } else
            if (fsize == 512*18*80*2) {
                type_id = "TYPE_FAT:PC-1440";
            } else
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid file size for DSK format"));

            // filesystem_id
            if (type_id == "TYPE_AGAT_140" || type_id == "TYPE_AGAT_840") {
                BYTES buffer(256);
                file->read (reinterpret_cast<char*>(buffer.data()), buffer.size());
                std::string ms = "MICROSOFT";
                std::string _ms(buffer.begin() + 0x74, buffer.begin() + 0x74 + ms.size());

                uint32_t vtoc_pos;
                if (type_id == "TYPE_AGAT_140") vtoc_pos=17*16*256;
                else
                if (type_id == "TYPE_AGAT_840") vtoc_pos=17*21*256;

                Agat_VTOC VTOC;
                file->seekg (vtoc_pos, std::ios::beg);
                file->read (reinterpret_cast<char*>(&VTOC), sizeof(Agat_VTOC));

                if (buffer[0] == 0x01 && buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else
                if (type_id == "TYPE_AGAT_140" && _ms == ms) {
                    if (ext == ".po")
                        filesystem_id = "FILESYSTEM_CPM_PRODOS";
                    else
                        filesystem_id = "FILESYSTEM_CPM_DOS";
                } else
                if (type_id == "TYPE_AGAT_140" && ext == ".cpm") {
                    filesystem_id = "FILESYSTEM_CPM_RAW";
                } else
                if (type_id == "TYPE_AGAT_140" && is_prodos_volume(*file, 35*16*256/PRODOS_BLOCK_SIZE)) {
                    // A 140 Kb image written in ProDOS block order
                    filesystem_id = "FILESYSTEM_PRODOS";
                } else
                if (type_id == "TYPE_AGAT_840" && is_onix_volume(*file, 860160 / ONIX_BLOCK_SIZE)) {
                    filesystem_id = "FILESYSTEM_ONIX";
                } else
                    filesystem_id = "FILESYSTEM_DOS33";
            } else
            if (type_id == "TYPE_AGAT_880" || type_id == "TYPE_OTHER:PRODOS-800") {
                filesystem_id = "FILESYSTEM_PRODOS";
            } else
            if (type_id == "TYPE_CPM:IRISHA-360-INT" || type_id == "TYPE_CPM:IRISHA-360-SEQ") {
                filesystem_id = "FILESYSTEM_CPM_RAW";
            } else
            if (type_id == "TYPE_CPM:GMD-7012") {
                filesystem_id = "FILESYSTEM_CPM_RAW";
            } else
            if (type_id.rfind("TYPE_FAT:", 0)==0) {
                filesystem_id = "FILESYSTEM_FAT";
            } else
            if (type_id.rfind("TYPE_RT11:", 0)==0) {
                filesystem_id = "FILESYSTEM_RT11";
            }
        } else
        if (ext == ".aim") {
            format_id = "FILE_AIM";

            type_id = "TYPE_AGAT_840";

            if (format_only) {
                filesystem_id = "";
                return Result::ok();
            }

            // The loader deduces the sector layout from the dump itself
            dsk_tools::LoaderAIM loader(file_name, format_id, "");
            BYTES buffer;
            Result res = loader.load(buffer);
            if (!res)
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Failed to load AIM file"));

            if (loader.get_sector_size() == static_cast<int>(PRODOS_BLOCK_SIZE)) {
                // 11 sectors of 512 bytes: an 880 Kb Nippel OS disk, ProDOS inside
                type_id = "TYPE_AGAT_880";
                BYTES block(buffer.begin() + PRODOS_ROOT_BLOCK * PRODOS_BLOCK_SIZE,
                            buffer.begin() + (PRODOS_ROOT_BLOCK + 1) * PRODOS_BLOCK_SIZE);
                if (!fsProDOS::volume_header_is_valid(block, buffer.size() / PRODOS_BLOCK_SIZE))
                    return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "ProDOS volume header not found"));
                filesystem_id = "FILESYSTEM_PRODOS";
            } else
            if (buffer[0] == 0x01) {
                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else
                if (fsOnix::volume_is_valid(buffer, buffer.size() / ONIX_BLOCK_SIZE)) {
                    filesystem_id = "FILESYSTEM_ONIX";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Failed to load AIM file"));
        } else
        if (ext == ".nic" || ext == ".nib" || ext == ".mfm") {
            if (ext == ".nib") {
                if (fsize == 232960)
                    type_id = "TYPE_AGAT_140";
                else
                if (fsize == 947520)
                    type_id = "TYPE_AGAT_840";
                else
                    return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid file size for NIB format"));
            } else
                type_id = "TYPE_AGAT_140";

            BYTES buffer;
            Result res;
            if (ext == ".nic") {
                format_id = "FILE_MFM_NIC";
                if (format_only) {
                    filesystem_id = "";
                    return Result::ok();
                }
                LoaderNIC loader(file_name, format_id, type_id);
                res = loader.load(buffer);
            } else
            if (ext == ".nib") {
                format_id = "FILE_MFM_NIB";
                if (format_only) {
                    filesystem_id = "";
                    return Result::ok();
                }
                LoaderNIB loader(file_name, format_id, type_id);
                res = loader.load(buffer);
            } else
            if (ext == ".mfm") {
                format_id = "FILE_HXC_MFM";
                if (format_only) {
                    filesystem_id = "";
                    return Result::ok();
                }
                LoaderHXC_MFM loader(file_name, format_id, type_id);
                res = loader.load(buffer);
            } else
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Unknown MFM format"));

            if (!res) return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Failed to load MFM file"));

            if (format_only) {
                filesystem_id = "";
                return Result::ok();
            }

            if (buffer[0] == 0x01) {
                std::string ms = "MICROSOFT";
                std::string _ms(buffer.begin() + 0x74, buffer.begin() + 0x74 + ms.size());

                if (buffer[2] == 0x58) {
                    filesystem_id = "FILESYSTEM_SPRITE_OS";
                } else
                if (type_id == "TYPE_AGAT_140" && _ms == ms) {
                    filesystem_id = "FILESYSTEM_CPM_DOS";
                } else
                if (fsOnix::volume_is_valid(buffer, buffer.size() / ONIX_BLOCK_SIZE)) {
                    filesystem_id = "FILESYSTEM_ONIX";
                } else {
                    filesystem_id = "FILESYSTEM_DOS33";
                }
            } else
                return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid filesystem signature"));
        } else
        if (ext == ".hfe") {
            format_id = "FILE_HXC_HFE";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return Result::ok();
            }

            BYTES whole(fsize);
            file->read(reinterpret_cast<char*>(whole.data()), whole.size());
            if (!file->good() || whole.size() < sizeof(HXC_HFE_HEADER))
                return Result::error(ErrorCode::LoadError, QT_TRANSLATE_NOOP("errors", "Cannot open HFE file"));
            const HXC_HFE_HEADER * hdr = reinterpret_cast<const HXC_HFE_HEADER*>(whole.data());

            // A disk of IBM MFM, IBM 3740 FM or DVK MX tracks is told by its
            // fields; anything else is left to the Agat
            HfeImage img;
            if (hfe_read(whole, img) && detect_hfe_tracks(img, type_id, filesystem_id))
                return Result::ok();

            if (hdr->number_of_side == 2 && hdr->number_of_track == 80) {
                type_id = "TYPE_AGAT_840";

                // Both Agat formats share the HFE header, the track data tells them apart
                {
                    LoaderHXC_HFE probe(file_name, format_id, type_id);
                    int sector_size = 256;
                    if (probe.probe_sector_size(sector_size) && sector_size == static_cast<int>(PRODOS_BLOCK_SIZE))
                        type_id = "TYPE_AGAT_880";
                }

                BYTES buffer(sizeof(HXC_HFE_HEADER));
                LoaderHXC_HFE loader(file_name, format_id, type_id);
                Result res = loader.load(buffer);

                if (type_id == "TYPE_AGAT_880") {
                    if (buffer.size() < (PRODOS_ROOT_BLOCK + 1) * PRODOS_BLOCK_SIZE)
                        return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid HFE file format"));
                    BYTES block(buffer.begin() + PRODOS_ROOT_BLOCK * PRODOS_BLOCK_SIZE,
                                buffer.begin() + (PRODOS_ROOT_BLOCK + 1) * PRODOS_BLOCK_SIZE);
                    if (!fsProDOS::volume_header_is_valid(block, buffer.size() / PRODOS_BLOCK_SIZE))
                        return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "ProDOS volume header not found"));
                    filesystem_id = "FILESYSTEM_PRODOS";
                } else
                if (res && buffer[0] == 0x01) {
                    if (buffer[2] == 0x58) {
                        filesystem_id = "FILESYSTEM_SPRITE_OS";
                    } else
                    if (fsOnix::volume_is_valid(buffer, buffer.size() / ONIX_BLOCK_SIZE)) {
                        filesystem_id = "FILESYSTEM_ONIX";
                    } else {
                        filesystem_id = "FILESYSTEM_DOS33";
                    }
                } else
                    return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Invalid HFE file format"));
            }
        } else
        if (ext == ".fil") {
            format_id = "FILE_FIL";
            type_id = "TYPE_FIL";
            filesystem_id = "FILESYSTEM_FIL";
        } else
        if (ext == ".imd") {
            format_id = "FILE_IMD";

            if (format_only) {
                type_id = "";
                filesystem_id = "";
                return Result::ok();
            }

            // TODO: make detection using contents
            type_id = "TYPE_CPM:IRISHA-360-INT";
            filesystem_id = "FILESYSTEM_CPM_RAW";
        } else
            return Result::error(ErrorCode::DetectError, QT_TRANSLATE_NOOP("errors", "Unknown file format"));

        return Result::ok();
    }

    std::string agat_vtoc_info(const Agat_VTOC & VTOC)
    {
        std::string result = "";
        if (VTOC.bytes_per_sector == 256 && VTOC.dos_release <= 3 ) {
            result += "{$VTOC_FOUND}\n";
        } else {
            result += "\n{$VTOC_NOT_FOUND}\n";
        }

        result += "    [$00]: $" + int_to_hex(VTOC._not_used_00) + " \n";
        result += "    {$VTOC_CATALOG_TRACK}: $" + int_to_hex(VTOC.catalog_track) + " (" + std::to_string(VTOC.catalog_track) + ") \n";
        result += "    {$VTOC_CATALOG_SECTOR}: $" + int_to_hex(VTOC.catalog_sector) + " (" + std::to_string(VTOC.catalog_sector) + ") \n";
        result += "    {$VTOC_DOS_RELEASE}: $" + int_to_hex(VTOC.dos_release) + " (" + std::to_string(VTOC.dos_release) + ") \n";
        result += "    [$04-05]: " + toHexList(&(VTOC._not_used_04[0]), 2, "$") + " \n";
        result += "    {$VTOC_VOLUME_ID}: $" + int_to_hex(VTOC.volume_id) + " (" + std::to_string(VTOC.volume_id) + ") \n";
        result += "    [$07]: $" + int_to_hex(VTOC._not_used_07) + " \n";
        result += "    {$VTOC_VOLUME_NAME}: «" + trim(agat_to_utf(VTOC.volume_name, 31)) + "» (" + toHexList(VTOC.volume_name, 31, "$") +")\n";
        result += "    {$VTOC_PAIRS_ON_SECTOR}: $" + int_to_hex(VTOC.pairs_on_sector) + " (" + std::to_string(VTOC.pairs_on_sector) + ") \n";
        result += "    [$28-2F]: " + toHexList(&(VTOC._not_used_28[0]), 8, "$") + " \n";
        result += "    {$VTOC_LAST_TRACK}: $" + int_to_hex(VTOC.last_track) + " (" + std::to_string(VTOC.last_track) + ") \n";
        result += "    {$VTOC_DIRECTION}: $" + int_to_hex(VTOC.direction) + " (" + std::to_string(VTOC.direction) + ") \n";
        result += "    [$32-33]: " + toHexList(&(VTOC._not_used_32[0]), 2, "$") + " \n";
        result += "    {$VTOC_TRACKS_TOTAL}: $" + int_to_hex(VTOC.tracks_total) + " (" + std::to_string(VTOC.tracks_total) + ") \n";
        result += "    {$VTOC_SECTORS_ON_TRACK}: $" + int_to_hex(VTOC.sectors_on_track) + " (" + std::to_string(VTOC.sectors_on_track) + ") \n";
        result += "    {$VTOC_BYTES_PER_SECTOR}: $" + int_to_hex(VTOC.bytes_per_sector) + " (" + std::to_string(VTOC.bytes_per_sector) + ") \n";

        return result;
    }

    std::string agat_sos_info(const SPRITE_OS_DPB_DISK & DPB)
    {
        std::string result = "";
        result += "{$DPB_INFO}\n";
        result += "    {$DPB_VOLUME_ID}: $" + int_to_hex(DPB.VOLUME) + " (" + std::to_string(DPB.VOLUME) + ") \n";
        result += "    {$DPB_TYPE}: $" + int_to_hex(DPB.TYPE) + " (" + std::to_string(DPB.TYPE) + ") \n";
        result += "    {$DPB_DSIDE}: $" + int_to_hex(DPB.DSIDE) + " (" + std::to_string(DPB.DSIDE) + ") \n";
        result += "    {$DPB_TSIZE}: $" + int_to_hex(DPB.TSIZE) + " (" + std::to_string(DPB.TSIZE) + ") \n";
        result += "    {$DPB_DSIZE}: $" + int_to_hex(DPB.DSIZE) + " (" + std::to_string(DPB.DSIZE) + ") \n";
        result += "    {$DPB_MAXBLOK}: $" + int_to_hex(DPB.MAXBLOK) + " (" + std::to_string(DPB.MAXBLOK) + ") \n";
        result += "    {$DPB_VTOCADR}: $" + int_to_hex(DPB.VTOCADR) + " (" + std::to_string(DPB.VTOCADR) + ") \n";
        return result;
    }

    std::pair<std::string, std::string> suggest_file_type(const std::string file_name, const BYTES & data)
    {
        std::string ext = get_file_ext(file_name);

        if (ext == ".spr") return {"PICTURE_VECTOR", "SPR"};

        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            const AGAT_EXIF_SECTOR * exif = reinterpret_cast<const AGAT_EXIF_SECTOR *>(data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR));
            if (exif->SIGNATURE[0] == 0xD6 && exif->SIGNATURE[1] == 0xD2) {
                // Agat images with an EXIF tail
                int mode_lo = exif->MODE & 0xF;
                int mode_hi = exif->MODE >> 4;
                if (mode_lo == 0) {
                    // Agat graphic modes
                    std::string type = "PICTURE_AGAT";
                    if (mode_hi == 1) return {type, "256x256x1"};
                    if (mode_hi == 4) return {type, "64x64x16"};
                    if (mode_hi == 5) return {type, "128x128x16"};
                    if (mode_hi == 6) return {type, "256x256x4"};
                    if (mode_hi == 7) return {type, "512x256x1"};
                    if (mode_hi == 8) return {type, "128x256x16"};
                    if (mode_hi == 9) return {type, "280x192HiRes"};
                } else
                if (mode_lo == 1) {
                    // Agat text modes
                    std::string type = "PICTURE_AGAT";
                    if (mode_hi == 2) return {type, "T32"};
                    if (mode_hi == 3) return {type, "T64"};
                } else
                if (mode_lo == 0xA) {
                    // Apple ][ graphic modes
                    std::string type = "PICTURE_APPLE";
                    if (mode_hi ==   9) return {type, "280x192HiRes"};
                    if (mode_hi == 0xC) return {type, "40x48LoRes"};
                    if (mode_hi == 0xD) return {type, "80x48DblLoRes"};
                    if (mode_hi == 0xE) return {type, "140x192DblHiRes"};
                    if (mode_hi == 0xF) return {type, "560x192DblHiResBW"};
                }
            }
        }
        std::vector<int> sizes_to_fit = {2048, 2048+256};
        if (std::find(sizes_to_fit.begin(), sizes_to_fit.end(), data.size()) != sizes_to_fit.end()) {
            std::string prefix = file_name.substr(0, 4);
            std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
            if ((prefix == "ZG9_") || (prefix == "ZG7_") || (prefix == "ZG9:") || (prefix == "ZG7:")) return {"PICTURE_AGAT", "FONT"};
        }

        return {"BINARY", ""};
    }

    std::string agat_vr_info(const BYTES & data, bool comment_only)
    {
        std::string result = "";

        if (data.size() > sizeof(AGAT_EXIF_SECTOR)) {
            const AGAT_EXIF_SECTOR * exif = reinterpret_cast<const AGAT_EXIF_SECTOR*>(data.data() + data.size() - sizeof(AGAT_EXIF_SECTOR));
            if (exif->SIGNATURE[0] == 0xD6 && exif->SIGNATURE[1] == 0xD2) {
                std::string comment = "";
                for (int i=0; i<12; i++) {
                    std::string line = agat_to_utf(&(exif->COMMENT[i*16]), 16);
                    comment += line + "\n";
                }

                if (comment_only) return trim(comment, " \t\n");

                result += "{$AGAT_VR_FOUND}:\n";

                // Comment
                result += "    {$AGAT_VR_COMMENT}:\n";
                result += "----------------\n";
                result += trim(comment, " \t\n") + "\n";
                result += "----------------\n";

                // Other data
                result += "    {$AGAT_VR_MODE}: $" + int_to_hex(exif->MODE) + " (";
                int mode_lo = exif->MODE & 0xF;
                int mode_hi = exif->MODE >> 4;
                result += std::string(agat_vr_mode_low[mode_lo]);
                switch (mode_lo) {
                case 0:
                    result += ", " + std::string(agat_vr_mode_high_agat_gr[mode_hi]);
                    break;
                case 1:
                    result += ", " + std::string(agat_vr_mode_high_agat_tx[mode_hi]);
                    break;
                case 10:
                    result += ", " + std::string(agat_vr_mode_high_apple[mode_hi]);
                    break;
                default:
                    break;
                }
                result += ")\n";
                if (mode_lo == 0 || mode_lo == 1) {
                    result += "    {$AGAT_VR_MAIN_PALETTE}: $" + int_to_hex(exif->PALETTE >> 4, false) + "\n";
                    result += "    {$AGAT_VR_ATL_PALETTE}: $" + int_to_hex(exif->PALETTE & 0xF, false) + "\n";
                    result += "    {$AGAT_VR_CUSTOM_PALETTE}:\n";
                    for (int i=0; i<8; i++) {

                        result += "        $" + int_to_hex(i*2, false) + ": #"
                            + int_to_hex((uint8_t)((exif->R[i] >> 4)*17))
                            + int_to_hex((uint8_t)((exif->G[i] >> 4)*17))
                            + int_to_hex((uint8_t)((exif->B[i] >> 4)*17))
                            + "\n";
                        result += "        $" + int_to_hex(i*2+1, false) + ": #"
                                  + int_to_hex((uint8_t)((exif->R[i] & 0xF)*17))
                                  + int_to_hex((uint8_t)((exif->G[i] & 0xF)*17))
                                  + int_to_hex((uint8_t)((exif->B[i] & 0xF)*17))
                                  + "\n";
                    }
                }
                if (mode_lo == 1) {
                    int font = exif->FONT >> 4;
                    result += "    {$AGAT_VR_FONT}: $" + int_to_hex(font, false) + " (" + std::string(agat_vr_font[font]) + ")\n";
                    if (font==15) {
                        result += "    {$AGAT_VR_CUSTOM_FONT}: " + trim(agat_to_utf(exif->FONT_NAME, 15)) + "\n";

                    }
                }
            }
            result += "\n";
        }

        return result;
    }

    std::unique_ptr<Writer> create_writer(const std::string & format_id, const uint8_t volume_id, diskImage * image) {
        std::set<std::string> mfm_formats = {"FILE_HXC_MFM", "FILE_MFM_NIB", "FILE_MFM_NIC"};

        if (mfm_formats.find(format_id) != mfm_formats.end())
           return make_unique<WriterHxCMFM>(format_id, image, volume_id);
        if (format_id == "FILE_HXC_HFE") return make_unique<dsk_tools::WriterHxCHFE>(format_id, image, volume_id);
        if (format_id == "FILE_RAW_MSB") return dsk_tools::make_unique<dsk_tools::WriterRAW>(format_id, image);

        return nullptr;
    }


    unsigned image_size_by_type(const std::string &type_id, const DiskFormatParams &format)
    {
        if (type_id == "TYPE_AGAT_840") return 2*80*21*256;
        if (type_id == "TYPE_AGAT_880") return 2*80*11*512;
        if (type_id == "TYPE_AGAT_140") return 1*35*16*256;
        if (type_id.rfind("TYPE_CPM:", 0)==0 || type_id.rfind("TYPE_FAT:", 0)==0 || type_id.rfind("TYPE_OTHER:", 0)==0 || type_id.rfind("TYPE_RT11:", 0)==0)
            return format.heads * format.tracks * format.sectors * format.sector_size;
        return 0;
    }

    static std::string diskdef_trim(const std::string &s)
    {
        const char *ws = " \t\r\n";
        size_t a = s.find_first_not_of(ws);
        if (a == std::string::npos) return std::string();
        size_t b = s.find_last_not_of(ws);
        return s.substr(a, b - a + 1);
    }

    DiskDefs parse_diskdefs(const std::string &contents)
    {
        DiskDefs result;
        DiskDef current;
        bool in_def = false;

        std::istringstream stream(contents);
        std::string line;

        while (std::getline(stream, line)) {
            std::string t = diskdef_trim(line);
            if (t.empty() || t[0] == '#') continue;

            std::string key, value;
            size_t sp = t.find_first_of(" \t");
            if (sp == std::string::npos) {
                key = t;
            } else {
                key = t.substr(0, sp);
                size_t vs = t.find_first_not_of(" \t", sp);
                value = (vs == std::string::npos) ? std::string() : diskdef_trim(t.substr(vs));
            }

            if (key == "diskdef") {
                current = DiskDef();
                current.name = value;
                in_def = true;
            } else if (key == "end") {
                if (in_def && !current.name.empty()) {
                    result[current.name] = current;
                }
                current = DiskDef();
                in_def = false;
            } else if (in_def) {
                if (key == "skewtab") {
                    current.skewtab.clear();
                    std::istringstream vs(value);
                    std::string num;
                    while (std::getline(vs, num, ',')) {
                        num = diskdef_trim(num);
                        if (num.empty()) continue;
                        try {
                            current.skewtab.push_back(std::stoi(num));
                        } catch (...) {
                            // Skip malformed entries
                        }
                    }
                } else if (key == "os" || key == "sides" || key == "charmap" || key == "layout") {
                    current.str_params[key] = value;
                } else {
                    try {
                        current.int_params[key] = std::stoi(value);
                    } catch (...) {
                        // Skip malformed entries
                    }
                }
            }
        }
        return result;
    }

} // namespace
