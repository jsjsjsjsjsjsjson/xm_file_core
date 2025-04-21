#ifndef XM_HELPER_H
#define XM_HELPER_H

#include <stdint.h>
#include <vector>

#define IS_COMPRESSED_MODE(mask) ((mask) & 0x80)
#define IS_ORIGINAL_MODE(mask)   (!((mask) & 0x80))
#define HAS_NOTE(mask)           (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x01)))
#define HAS_INSTRUMENT(mask)     (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x02)))
#define HAS_VOLUME(mask)         (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x04)))
#define HAS_EFFECT_TYPE(mask)    (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x08)))
#define HAS_EFFECT_PARAM(mask)   (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x10)))

extern const char note_table[12][3];

typedef struct {
    uint16_t x = 0;
    int16_t y = 0;
} env_point_t;

typedef struct __attribute__((packed)) {
    bool on : 1;
    bool sus : 1;
    bool loop : 1;
} env_type_t;

typedef struct __attribute__((packed)) {
    uint8_t loop_mode : 2; // 0 = No Loop, 1 = Forward Loop, 2 = Ping-pong
    uint8_t reserved : 2;
    bool sample_bit : 1; // false = 8bit sample, true = 16bit sample
} sample_type_t;

void xm_note_to_str(uint8_t note, char output[4]);
void parse_vol_cmd(uint8_t vol_cmd, char* mnemonic, uint8_t* val);
size_t encode_dpcm_8bit(int8_t* pcm_data, int8_t* dpcm_data, size_t num_samples);
size_t encode_dpcm_16bit(int16_t* pcm_data, int16_t* dpcm_data, size_t num_samples);
void decode_dpcm_8bit(int8_t* dpcm_data, int8_t* pcm_data, size_t num_samples);
void decode_dpcm_16bit(int16_t* dpcm_data, int16_t* pcm_data, size_t num_samples);
void genEnvTable(const env_point_t* env_points, uint8_t num_points, std::vector<int16_t>& table);

#endif