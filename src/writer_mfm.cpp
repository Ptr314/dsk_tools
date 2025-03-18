#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "writer_mfm.h"

namespace dsk_tools {

WriterMFM::WriterMFM(const std::string & format_id, diskImage * image_to_save, const uint8_t volume_id, const std::string &interleaving_id):
        Writer(format_id, image_to_save)
        , m_volume_id(volume_id)
        , m_interleaving_id(interleaving_id)
    {}

    int WriterMFM::sector_raw2logic(int sector)
    {
        if (m_interleaving_id == "INTERLEAVING_DOS33") {
            if (sector < 16)
                return agat_140_raw2logic[sector];
            else
                throw std::runtime_error("DOS33 sector inteleaving index overflow");
        } else
        if (m_interleaving_id == "INTERLEAVING_PRODOS") {
            if (sector < 16)
                return prodos_raw2logic[sector];
            else
                throw std::runtime_error("ProDOS sector inteleaving index overflow");
        } else
        if (m_interleaving_id == "INTERLEAVING_OFF")
            return sector;
        else
            throw std::runtime_error("Unknown interleaving type");
    }

    void WriterMFM::write_gcr62_track(BYTES & out, uint8_t track, int track_length)
    {
        BYTES bytes;
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
            uint8_t volume = m_volume_id;
            uint8_t sector_t = sector_raw2logic(sector);
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
            uint8_t * data = image->get_sector_data(head, track, sector_t);
            encode_gcr62(data, encoded_sector);
            out.insert(out.end(), &encoded_sector[0], &encoded_sector[343]);      // +343
            // Epilogue
            bytes = {0xDE, 0xAA, 0xEB};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());   // +3
            // GAP 2
            out.insert(out.end(), AGAT_140_GAP2, 0xFF);                         // +27
        }
        // GAP 3
        out.insert(out.end(), AGAT_140_GAP3, 0xFF);                             // Padding until track_length

    }

    void WriterMFM::write_gcr62_nic_track(BYTES &out, uint8_t track)
    {
        BYTES bytes;
        uint8_t encoded_sector[344];                                            // 343+1

        int head = 0;
        // Agat counts sectors from 0
        for (uint8_t sector = 0; sector < image->get_sectors(); sector++) {
            // GAP
            out.insert(out.end(), 22, 0xFF);
            // ?
            bytes = {0x03,0xfc,0xff,0x3f,0xcf,0xf3,0xfc,0xff,0x3f,0xcf,0xf3,0xfc};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());
            // Prologue
            bytes = {0xD5, 0xAA, 0x96};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());
            // Address
            uint8_t volume = m_volume_id;
            uint8_t sector_t = sector_raw2logic(sector);
            BYTES address_field = {volume, track, sector_t, static_cast<uint8_t>(volume ^ track ^ sector_t)};
            bytes = code44(address_field);
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());
            // Epilogue
            bytes = {0xDE, 0xAA, 0xEB};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());
            // GAP
            out.insert(out.end(), 5, 0xFF);
            // Data field
            // Prologue
            bytes = {0xD5, 0xAA, 0xAD};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());

            // Data + CRC
            uint8_t * data = image->get_sector_data(head, track, sector_t);
            encode_gcr62(data, encoded_sector);
            out.insert(out.end(), &encoded_sector[0], &encoded_sector[343]);
            // Epilogue
            bytes = {0xDE, 0xAA, 0xEB};
            out.insert(out.end(), bytes.data(), bytes.data() + bytes.size());
            //GAP
            out.insert(out.end(), 14, 0xFF);
            // Sector padding
            out.insert(out.end(), 512-416, 0);
        }
    }

    void WriterMFM::write_agat840_track(BYTES &out, uint8_t head, uint8_t track)
    {
        uint8_t last_byte = 0;
        // GAP 0
        encode_agat_mfm_array(out, 0xAA, 144, last_byte);
        for (uint8_t sector = 0; sector < image->get_sectors(); sector++) {
            // Desync
            out.push_back(0x22);                                        // 0
            out.push_back(0x09);                                        // 1
            last_byte = 0xA4;
            encode_agat_mfm_array(out, 0xFF, 1, last_byte);             // 2-3
            // Index start
            encode_agat_mfm_array(out, 0x95, 1, last_byte);             // 4-5
            encode_agat_mfm_array(out, 0x6A, 1, last_byte);             // 6-7
            // VTS
            encode_agat_mfm_array(out, m_volume_id, 1, last_byte); //Volume    // 8-9
            encode_agat_mfm_array(out, track*2 + head, 1, last_byte);   // A-B
            uint8_t sector_t = sector_raw2logic(sector);
            encode_agat_mfm_array(out, sector_t, 1, last_byte);
            // Index end
            encode_agat_mfm_array(out, 0x5A, 1, last_byte);
            // GAP
            encode_agat_mfm_array(out, 0xAA, 3, last_byte);
            // Desync
            out.push_back(0x22);
            out.push_back(0x09);
            last_byte = 0xA4;
            encode_agat_mfm_array(out, 0xFF, 1, last_byte);
            // Data mark
            encode_agat_mfm_array(out, 0x6A, 1, last_byte);
            encode_agat_mfm_array(out, 0x95, 1, last_byte);
            // Data + crc
            uint8_t * data = image->get_sector_data(0, track*2 + head, sector_t);
            uint8_t crc = encode_agat_mfm_data(out, data, 256, last_byte);
            encode_agat_mfm_array(out, crc, 1, last_byte);
            // Data end
            encode_agat_mfm_array(out, 0x5A, 1, last_byte);
            // GAP
            encode_agat_mfm_array(out, 0xAA, 29, last_byte);
        }
        // GAP
        encode_agat_mfm_array(out, 0xAA, 20, last_byte);
        // Fill until standard hfe track length
        encode_agat_mfm_array(out, 0xAA, (HFE_TRACK_LEN/2 - (144 + 302*image->get_sectors() + 20)*2)/2, last_byte);

    }
}
