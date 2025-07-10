// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Main header file

#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H



#include <string>
#include <sstream>

#include "definitions.h"
#include "utils.h"

#include "disk_image.h"
#include "image_agat140.h"
#include "image_agat840.h"
#include "image_fil.h"

#include "loader.h"
#include "loader_raw.h"
#include "loader_aim.h"
#include "loader_hxc_hfe.h"
#include "loader_fil.h"
#include "loader_nib.h"
#include "loader_nic.h"
#include "loader_hxc_mfm.h"

#include "writer.h"
#include "writer_raw.h"
#include "writer_mfm.h"
#include "writer_hxc_hfe.h"
#include "writer_hxc_mfm.h"

#include "filesystem.h"
#include "fs_dos33.h"
#include "fs_spriteos.h"
#include "fs_cpm.h"
#include "fs_fil.h"

#include "viewers/viewer.h"
#include "viewers/viewer_binary.h"
#include "viewers/viewer_text.h"
#include "viewers/viewer_basic_agat.h"
#include "viewers/viewer_basic_apple.h"
#include "viewers/viewer_basic_mbasic.h"
#include "viewers/viewer_pics_agat.h"


namespace dsk_tools {

    int detect_fdd_type(const std::string &file_name, std::string &format_id, std::string &type_id, std::string &filesystem_id, bool format_only = false);
    dsk_tools::diskImage * prepare_image(std::string file_name, std::string format_id, std::string type_id);
    dsk_tools::fileSystem * prepare_filesystem(dsk_tools::diskImage * image, std::string filesystem_id);
    BYTES code44(const BYTES & buffer);
    BYTES decode44(const BYTES & buffer);
    void encode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    bool decode_gcr62(const uint8_t data_in[], uint8_t * data_out);
    uint16_t encode_agat_MFM_byte(uint8_t data, uint8_t &last_byte);
    uint8_t decode_agat_MFM_byte(uint8_t data);
    void encode_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t & last_byte);
    uint8_t encode_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t & last_byte);
    void decode_agat_mfm_data(BYTES &out, const BYTES & in);
    int decode_agat_840_track(BYTES &out, const BYTES & in);
    int decode_agat_840_image(BYTES &out, const BYTES & in);
    std::string agat_vtoc_info(const Agat_VTOC & VTOC);
    std::string agat_sos_info(const SPRITE_OS_DPB_DISK & DPB);
    std::pair<std::string, std::string> suggest_file_type(const std::string file_name, const BYTES & data);
    std::string agat_vr_info(const BYTES & data);

    int load_agat140_track(int track, BYTES & buffer, const BYTES & in, int track_len);

    void register_all_viewers();

} // namespace

#endif /* DSK_TOOLS_H */
