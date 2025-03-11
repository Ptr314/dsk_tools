#ifndef WRITER_MFM_H
#define WRITER_MFM_H

#include "writer.h"

namespace dsk_tools {

    #define AGAT_140_GAP0    48
    #define AGAT_140_GAP1    6
    #define AGAT_140_GAP2    27
    #define AGAT_140_GAP3    (track_length - (AGAT_140_GAP0 + 16 * (3 + 8 + 3 + AGAT_140_GAP1 + 3 + 343 + 3 + AGAT_140_GAP2)))

    static const uint16_t agat_MFM_tab[]=
        {
                0x5555, 0x5595, 0x5525, 0x55A5, 0x5549, 0x5589, 0x5529, 0x55A9,
                0x5552, 0x5592, 0x5522, 0x55A2, 0x554A, 0x558A, 0x552A, 0x55AA,
                0x9554, 0x9594, 0x9524, 0x95A4, 0x9548, 0x9588, 0x9528, 0x95A8,
                0x9552, 0x9592, 0x9522, 0x95A2, 0x954A, 0x958A, 0x952A, 0x95AA,
                0x2555, 0x2595, 0x2525, 0x25A5, 0x2549, 0x2589, 0x2529, 0x25A9,
                0x2552, 0x2592, 0x2522, 0x25A2, 0x254A, 0x258A, 0x252A, 0x25AA,
                0xA554, 0xA594, 0xA524, 0xA5A4, 0xA548, 0xA588, 0xA528, 0xA5A8,
                0xA552, 0xA592, 0xA522, 0xA5A2, 0xA54A, 0xA58A, 0xA52A, 0xA5AA,
                0x4955, 0x4995, 0x4925, 0x49A5, 0x4949, 0x4989, 0x4929, 0x49A9,
                0x4952, 0x4992, 0x4922, 0x49A2, 0x494A, 0x498A, 0x492A, 0x49AA,
                0x8954, 0x8994, 0x8924, 0x89A4, 0x8948, 0x8988, 0x8928, 0x89A8,
                0x8952, 0x8992, 0x8922, 0x89A2, 0x894A, 0x898A, 0x892A, 0x89AA,
                0x2955, 0x2995, 0x2925, 0x29A5, 0x2949, 0x2989, 0x2929, 0x29A9,
                0x2952, 0x2992, 0x2922, 0x29A2, 0x294A, 0x298A, 0x292A, 0x29AA,
                0xA954, 0xA994, 0xA924, 0xA9A4, 0xA948, 0xA988, 0xA928, 0xA9A8,
                0xA952, 0xA992, 0xA922, 0xA9A2, 0xA94A, 0xA98A, 0xA92A, 0xA9AA,
                0x5255, 0x5295, 0x5225, 0x52A5, 0x5249, 0x5289, 0x5229, 0x52A9,
                0x5252, 0x5292, 0x5222, 0x52A2, 0x524A, 0x528A, 0x522A, 0x52AA,
                0x9254, 0x9294, 0x9224, 0x92A4, 0x9248, 0x9288, 0x9228, 0x92A8,
                0x9252, 0x9292, 0x9222, 0x92A2, 0x924A, 0x928A, 0x922A, 0x92AA,
                0x2255, 0x2295, 0x2225, 0x22A5, 0x2249, 0x2289, 0x2229, 0x22A9,
                0x2252, 0x2292, 0x2222, 0x22A2, 0x224A, 0x228A, 0x222A, 0x22AA,
                0xA254, 0xA294, 0xA224, 0xA2A4, 0xA248, 0xA288, 0xA228, 0xA2A8,
                0xA252, 0xA292, 0xA222, 0xA2A2, 0xA24A, 0xA28A, 0xA22A, 0xA2AA,
                0x4A55, 0x4A95, 0x4A25, 0x4AA5, 0x4A49, 0x4A89, 0x4A29, 0x4AA9,
                0x4A52, 0x4A92, 0x4A22, 0x4AA2, 0x4A4A, 0x4A8A, 0x4A2A, 0x4AAA,
                0x8A54, 0x8A94, 0x8A24, 0x8AA4, 0x8A48, 0x8A88, 0x8A28, 0x8AA8,
                0x8A52, 0x8A92, 0x8A22, 0x8AA2, 0x8A4A, 0x8A8A, 0x8A2A, 0x8AAA,
                0x2A55, 0x2A95, 0x2A25, 0x2AA5, 0x2A49, 0x2A89, 0x2A29, 0x2AA9,
                0x2A52, 0x2A92, 0x2A22, 0x2AA2, 0x2A4A, 0x2A8A, 0x2A2A, 0x2AAA,
                0xAA54, 0xAA94, 0xAA24, 0xAAA4, 0xAA48, 0xAA88, 0xAA28, 0xAAA8,
                0xAA52, 0xAA92, 0xAA22, 0xAAA2, 0xAA4A, 0xAA8A, 0xAA2A, 0xAAAA,
                0x5455, 0x5495, 0x5425, 0x54A5, 0x5449, 0x5489, 0x5429, 0x54A9,
                0x5452, 0x5492, 0x5422, 0x54A2, 0x544A, 0x548A, 0x542A, 0x54AA,
                0x9454, 0x9494, 0x9424, 0x94A4, 0x9448, 0x9488, 0x9428, 0x94A8,
                0x9452, 0x9492, 0x9422, 0x94A2, 0x944A, 0x948A, 0x942A, 0x94AA,
                0x2455, 0x2495, 0x2425, 0x24A5, 0x2449, 0x2489, 0x2429, 0x24A9,
                0x2452, 0x2492, 0x2422, 0x24A2, 0x244A, 0x248A, 0x242A, 0x24AA,
                0xA454, 0xA494, 0xA424, 0xA4A4, 0xA448, 0xA488, 0xA428, 0xA4A8,
                0xA452, 0xA492, 0xA422, 0xA4A2, 0xA44A, 0xA48A, 0xA42A, 0xA4AA,
                0x4855, 0x4895, 0x4825, 0x48A5, 0x4849, 0x4889, 0x4829, 0x48A9,
                0x4852, 0x4892, 0x4822, 0x48A2, 0x484A, 0x488A, 0x482A, 0x48AA,
                0x8854, 0x8894, 0x8824, 0x88A4, 0x8848, 0x8888, 0x8828, 0x88A8,
                0x8852, 0x8892, 0x8822, 0x88A2, 0x884A, 0x888A, 0x882A, 0x88AA,
                0x2855, 0x2895, 0x2825, 0x28A5, 0x2849, 0x2889, 0x2829, 0x28A9,
                0x2852, 0x2892, 0x2822, 0x28A2, 0x284A, 0x288A, 0x282A, 0x28AA,
                0xA854, 0xA894, 0xA824, 0xA8A4, 0xA848, 0xA888, 0xA828, 0xA8A8,
                0xA852, 0xA892, 0xA822, 0xA8A2, 0xA84A, 0xA88A, 0xA82A, 0xA8AA,
                0x5255, 0x5295, 0x5225, 0x52A5, 0x5249, 0x5289, 0x5229, 0x52A9,
                0x5252, 0x5292, 0x5222, 0x52A2, 0x524A, 0x528A, 0x522A, 0x52AA,
                0x9254, 0x9294, 0x9224, 0x92A4, 0x9248, 0x9288, 0x9228, 0x92A8,
                0x9252, 0x9292, 0x9222, 0x92A2, 0x924A, 0x928A, 0x922A, 0x92AA,
                0x2255, 0x2295, 0x2225, 0x22A5, 0x2249, 0x2289, 0x2229, 0x22A9,
                0x2252, 0x2292, 0x2222, 0x22A2, 0x224A, 0x228A, 0x222A, 0x22AA,
                0xA254, 0xA294, 0xA224, 0xA2A4, 0xA248, 0xA288, 0xA228, 0xA2A8,
                0xA252, 0xA292, 0xA222, 0xA2A2, 0xA24A, 0xA28A, 0xA22A, 0xA2AA,
                0x4A55, 0x4A95, 0x4A25, 0x4AA5, 0x4A49, 0x4A89, 0x4A29, 0x4AA9,
                0x4A52, 0x4A92, 0x4A22, 0x4AA2, 0x4A4A, 0x4A8A, 0x4A2A, 0x4AAA,
                0x8A54, 0x8A94, 0x8A24, 0x8AA4, 0x8A48, 0x8A88, 0x8A28, 0x8AA8,
                0x8A52, 0x8A92, 0x8A22, 0x8AA2, 0x8A4A, 0x8A8A, 0x8A2A, 0x8AAA,
                0x2A55, 0x2A95, 0x2A25, 0x2AA5, 0x2A49, 0x2A89, 0x2A29, 0x2AA9,
                0x2A52, 0x2A92, 0x2A22, 0x2AA2, 0x2A4A, 0x2A8A, 0x2A2A, 0x2AAA,
                0xAA54, 0xAA94, 0xAA24, 0xAAA4, 0xAA48, 0xAA88, 0xAA28, 0xAAA8,
                0xAA52, 0xAA92, 0xAA22, 0xAAA2, 0xAA4A, 0xAA8A, 0xAA2A, 0xAAAA
    };


    class WriterMFM:public Writer
    {
    protected:
        uint8_t m_volume_id;
        std::string m_interleaving_id;

        std::vector<int> sector_translation;

        void write_gcr62_track(BYTES &out, uint8_t track, int track_length);
        void write_gcr62_nic_track(BYTES &out, uint8_t track);
        void write_agat840_track(BYTES &out, uint8_t head, uint8_t track);
        uint16_t encode_agat_MFM_byte(uint8_t data, uint8_t * last_byte);
        void write_agat_mfm_array(BYTES &out, uint8_t data, uint16_t count, uint8_t * last_byte);
        uint8_t write_agat_mfm_data(BYTES &out, uint8_t * data, uint16_t count, uint8_t * last_byte);
    public:
        virtual int sector_raw2logic(int sector) override;
        WriterMFM(const std::string & format_id, diskImage *image_to_save, const uint8_t volume_id, const std::string & interleaving_id);

    };

}

#endif // WRITER_MFM_H
