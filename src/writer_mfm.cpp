#include <cstring>

#include "dsk_tools/dsk_tools.h"
#include "writer_mfm.h"

namespace dsk_tools {

    WriterMFM::WriterMFM(const std::string & format_id, diskImage * image_to_save):
        Writer(format_id, image_to_save)
    {}

    uint16_t WriterMFM::encode_agat_MFM_byte(uint8_t data, uint8_t * last_byte)
    {
        uint16_t mfm_encoded;
        mfm_encoded = agat_MFM_tab[((*last_byte & 1) << 8) + data];

        *last_byte = data;

        return (mfm_encoded << 8) + (mfm_encoded >> 8);
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
            uint8_t volume = 0xFE;
            uint8_t sector_t = image->translate_sector_raw2logic(sector);
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
            uint8_t * data = image->get_raw_sector_data(head, track, sector);
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

    void WriterMFM::write_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t * last_byte)
    {
        uint16_t mfm_word;
        for (int i=0; i<count; i++) {
            mfm_word = encode_agat_MFM_byte(data, &*last_byte);
            out.push_back(mfm_word & 0xFF);
            out.push_back((mfm_word >> 8) & 0xFF);
        }
    }

    uint8_t WriterMFM::write_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t * last_byte)
    {
        uint16_t mfm_word;
        uint16_t crc = 0;
        for (int i=0; i<count; i++) {
            mfm_word = encode_agat_MFM_byte(data[i], &*last_byte);
            out.push_back(mfm_word & 0xFF);
            out.push_back((mfm_word >> 8) & 0xFF);
            if (crc > 0xFF) crc = (crc + 1) & 0xFF;
            crc += data[i];
        }
        return crc & 0xFF;
    }

    void WriterMFM::write_agat840_track(BYTES &out, uint8_t head, uint8_t track)
    {
        uint8_t last_byte = 0;
        // GAP 0
        write_agat_mfm_array(out, 0xAA, 144, &last_byte);
        for (uint8_t sector = 0; sector < image->get_sectors(); sector++) {
            // Desync
            out.push_back(0x22);                                        // 0
            out.push_back(0x09);                                        // 1
            last_byte = 0xA4;
            write_agat_mfm_array(out, 0xFF, 1, &last_byte);             // 2-3
            // Index start
            write_agat_mfm_array(out, 0x95, 1, &last_byte);             // 4-5
            write_agat_mfm_array(out, 0x6A, 1, &last_byte);             // 6-7
            // VTS
            write_agat_mfm_array(out, 0xFE, 1, &last_byte); //Volume    // 8-9
            write_agat_mfm_array(out, track*2 + head, 1, &last_byte);   // A-B
            write_agat_mfm_array(out, sector, 1, &last_byte);
            // Index end
            write_agat_mfm_array(out, 0x5A, 1, &last_byte);
            // GAP
            write_agat_mfm_array(out, 0xAA, 3, &last_byte);
            // Desync
            out.push_back(0x22);
            out.push_back(0x09);
            last_byte = 0xA4;
            write_agat_mfm_array(out, 0xFF, 1, &last_byte);
            // Data mark
            write_agat_mfm_array(out, 0x6A, 1, &last_byte);
            write_agat_mfm_array(out, 0x95, 1, &last_byte);
            // Data + crc
            uint8_t * data = image->get_raw_sector_data(head, track, sector);
            uint8_t crc = write_agat_mfm_data(out, data, 256, &last_byte);
            write_agat_mfm_array(out, crc, 1, &last_byte);
            // Data end
            write_agat_mfm_array(out, 0x5A, 1, &last_byte);
            // GAP
            write_agat_mfm_array(out, 0xAA, 29, &last_byte);
        }
        // GAP
        write_agat_mfm_array(out, 0xAA, 20, &last_byte);
        // Fill until standard hfe track length
        write_agat_mfm_array(out, 0xAA, (HFE_TRACK_LEN/2 - (144 + 302*image->get_sectors() + 20)*2)/2, &last_byte);

    }
}
