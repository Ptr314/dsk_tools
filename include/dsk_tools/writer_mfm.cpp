#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "dsk_tools/writer_mfm.h"

namespace dsk_tools {

    WriterMFM::WriterMFM(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    void WriterMFM::write_gcr62_track(std::vector<uint8_t> & out, uint8_t track)
    {
        std::vector<uint8_t> bytes;
        uint8_t encoded_sector[344];                                            // 343+1

        // GAP 0
        out.insert(out.end(), AGAT_140_GAP0, 0xFF);                             // +48

        int head = 0;
        // Agat counts sectors from 0
        for (uint8_t sector = 0; sector < image->get_sectors(); sector++) {
            // Prologue
            bytes = {0xD5, 0xAA, 0x96};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +3
            // Address
            uint8_t volume = 0xFE;
            uint8_t sector_t = image->translate_sector_raw2logic(sector);
            BYTES address_field = {volume, track, sector_t, static_cast<uint8_t>(volume ^ track ^ sector_t)};
            bytes = code44(address_field);
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +8
            // Epilogue
            bytes = {0xDE, 0xAA, 0xEB};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +3
            // GAP 1
            out.insert(out.end(), AGAT_140_GAP1, 0xFF);                         // +6
            // Data field
            // Prologue
            bytes = {0xD5, 0xAA, 0xAD};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +3
            // Data + CRC
            uint8_t * data = image->get_raw_sector_data(head, track, sector);
            encode_gcr62(data, encoded_sector);
            out.insert(out.end(), &encoded_sector[0], &encoded_sector[343]);      // +343
            // Epilogue
            bytes = {0xDE, 0xAA, 0xEB};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +3
            // GAP 2
            out.insert(out.end(), AGAT_140_GAP2, 0xFF);                         // +27
        }
        // GAP 3
        out.insert(out.end(), AGAT_140_GAP3, 0xFF);                             // +16

    }
}
