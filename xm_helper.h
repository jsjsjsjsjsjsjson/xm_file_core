#ifndef XM_HELPER_H
#define XM_HELPER_H

#include <stdint.h>

#define IS_COMPRESSED_MODE(mask) ((mask) & 0x80)
#define IS_ORIGINAL_MODE(mask)   (!((mask) & 0x80))
#define HAS_NOTE(mask)           (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x01)))
#define HAS_INSTRUMENT(mask)     (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x02)))
#define HAS_VOLUME(mask)         (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x04)))
#define HAS_EFFECT_TYPE(mask)    (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x08)))
#define HAS_EFFECT_PARAM(mask)   (IS_ORIGINAL_MODE(mask) || (((mask) & 0x80) && ((mask) & 0x10)))

const char* note_table[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};

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

void xm_note_to_str(uint8_t note, char output[4]) {
    if (note) {
        note--;
        int8_t i = note % 12;
        output[0] = note_table[i][0];
        output[1] = note_table[i][1];
        output[2] = 49 + (note / 12);
        output[3] = '\0';
    } else {
        output[0] = '.';
        output[1] = '.';
        output[2] = '.';
        output[3] = '\0';
    }
}

void parse_vol_cmd(uint8_t vol_cmd, char* mnemonic, uint8_t* val) {
    if (!mnemonic || !val) return;
    if (vol_cmd >= 0x10 && vol_cmd <= 0x4F) {
        *mnemonic = 'v';
        *val = vol_cmd - 0x10;
    } else if (vol_cmd >= 0x60 && vol_cmd <= 0x6F) {
        *mnemonic = 'd';
        *val = vol_cmd - 0x60;
    } else if (vol_cmd >= 0x70 && vol_cmd <= 0x7F) {
        *mnemonic = 'c';
        *val = vol_cmd - 0x70;
    } else if (vol_cmd >= 0x80 && vol_cmd <= 0x8F) {
        *mnemonic = 'b';
        *val = vol_cmd - 0x80;
    } else if (vol_cmd >= 0x90 && vol_cmd <= 0x9F) {
        *mnemonic = 'a';
        *val = vol_cmd - 0x90;
    } else if (vol_cmd >= 0xA0 && vol_cmd <= 0xAF) {
        *mnemonic = 'u';
        *val = vol_cmd - 0xA0;
    } else if (vol_cmd >= 0xB0 && vol_cmd <= 0xBF) {
        *mnemonic = 'h';
        *val = vol_cmd - 0xB0;
    } else if (vol_cmd >= 0xC0 && vol_cmd <= 0xCF) {
        *mnemonic = 'p';
        *val = vol_cmd - 0xC0;
    } else if (vol_cmd >= 0xD0 && vol_cmd <= 0xDF) {
        *mnemonic = 'l';
        *val = vol_cmd - 0xD0;
    } else if (vol_cmd >= 0xE0 && vol_cmd <= 0xEF) {
        *mnemonic = 'r';
        *val = vol_cmd - 0xE0;
    } else if (vol_cmd >= 0xF0 && vol_cmd <= 0xFF) {
        *mnemonic = 'g';
        *val = vol_cmd - 0xF0;
    } else {
        *mnemonic = '\0';
        *val = 0;
    }
}

size_t encode_dpcm_8bit(int8_t* pcm_data, int8_t* dpcm_data, size_t num_samples) {
    dpcm_data[0] = pcm_data[0];
    int16_t accumulated_error = 0;
    size_t error_count = 0;

    for (size_t i = 1; i < num_samples; ++i) {
        int16_t diff = pcm_data[i] - pcm_data[i - 1] + accumulated_error;

        if (diff > 127) {
            accumulated_error = diff - 127;
            diff = 127;
            error_count++;
        } else if (diff < -128) {
            accumulated_error = diff + 128;
            diff = -128;
            error_count++;
        } else {
            accumulated_error = 0;
        }

        dpcm_data[i] = (int8_t)diff;
    }
    return error_count;
}

size_t encode_dpcm_16bit(int16_t* pcm_data, int16_t* dpcm_data, size_t num_samples) {
    dpcm_data[0] = pcm_data[0];
    int32_t accumulated_error = 0;
    size_t error_count = 0;

    for (size_t i = 1; i < num_samples; ++i) {
        int32_t diff = pcm_data[i] - pcm_data[i - 1] + accumulated_error;

        if (diff > 32767) {
            accumulated_error = diff - 32767;
            diff = 32767;
            error_count++;
        } else if (diff < -32768) {
            accumulated_error = diff + 32768;
            diff = -32768;
            error_count++;
        } else {
            accumulated_error = 0;
        }

        dpcm_data[i] = (int16_t)diff;
    }
    return error_count;
}

void decode_dpcm_8bit(int8_t* dpcm_data, int8_t* pcm_data, size_t num_samples) {
    pcm_data[0] = dpcm_data[0];
    for (size_t i = 1; i < num_samples; ++i) {
        pcm_data[i] = pcm_data[i - 1] + dpcm_data[i];
    }
}

void decode_dpcm_16bit(int16_t* dpcm_data, int16_t* pcm_data, size_t num_samples) {
    pcm_data[0] = dpcm_data[0];
    for (size_t i = 1; i < num_samples; ++i) {
        pcm_data[i] = pcm_data[i - 1] + dpcm_data[i];
    }
}

void genEnvTable(const env_point_t* env_points, uint8_t num_points, std::vector<int16_t>& table) {
    if (num_points < 2 || num_points > 12) {
        table.clear();
        return;
    }

    uint16_t total_size = 0;
    for (uint8_t i = 0; i < num_points - 1; ++i) {
        total_size += (env_points[i + 1].x - env_points[i].x);
    }
    table.resize(total_size + (num_points - 1));

    uint16_t index = 0;

    for (uint8_t i = 0; i < num_points - 1; ++i) {
        const env_point_t& p0 = env_points[i];
        const env_point_t& p1 = env_points[i + 1];

        uint16_t x0 = p0.x;
        int16_t y0 = p0.y;
        uint16_t x1 = p1.x;
        int16_t y1 = p1.y;

        uint16_t segment_length = x1 - x0;

        int32_t delta_y = y1 - y0;  
        int32_t inc = delta_y * (1 << 16) / segment_length;

        for (uint16_t j = 0; j <= segment_length; ++j) {
            int16_t y_interpolated = y0 + (inc * j >> 16);
            table[index++] = y_interpolated;
        }
    }
}

#endif