#include <cstring>

#include "dsk_tools/writer_mfm.h"

namespace dsk_tools {

    WriterMFM::WriterMFM(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    void WriterMFM::write_gcr62_track(std::vector<uint8_t> & out, uint8_t track)
    {
        uint8_t gap_bytes[256];
        memset(&gap_bytes, 0xFF, sizeof(gap_bytes));

        uint8_t encoded_sector[344];

        // GAP 0
        out.insert(out.end(), AGAT_140_GAP0, 0xFF);

        // Agat counts sectors from 0
        for (uint8_t sector = 0; sector < image->get_sectors(); sector++) {
            // Prologue
            std::vector<uint8_t> pro = {0xD5, 0xAA, 0x96};
            out.insert(out.end(), pro.data(), pro.data() + pro.size());
            // Address
            uint8_t volume = 0xFE;
            uint8_t sector_t = image->translate_sector(sector);
            uint8_t address_field[4] = {volume, track, sector_t, static_cast<uint8_t>(volume ^ track ^ sector_t)};
            out->write(code44(address_field, 4));
        }

    }
}
