#ifndef XM_FILE_H
#define XM_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

#include "xm_helper.h"

typedef struct {
    char id[17] = {'E', 'x', 't', 'e', 'n', 'd', 'e', 'd', ' ', 'm', 'o', 'd', 'u', 'l', 'e', ':', ' '};
    char name[20] = "New Module";
    uint8_t X1A = 0x1A;
    char trkname[20] = "SlowTracker v2.01";
    uint16_t version = 0x0104;
} xm_metadata_t;

typedef struct {
    uint32_t size = 20 + 256;
    uint16_t songLength = 1;
    uint16_t resetVector = 0;
    uint16_t numChannels = 4;
    uint16_t numPatterns = 1;
    uint16_t numInstruments = 1;
    uint16_t freqMode = 1; // 1 = Liner, 0 = Amiga
    uint16_t defaultTempo = 2;
    uint16_t defaultBPM = 150;

    std::vector<uint8_t> orderTable;
} xm_header_t;

typedef struct {
    uint8_t mask = 0;
    uint8_t note = 0;
    uint8_t inst = 0;
    uint8_t vol = 0;
    uint8_t fx_cmd = 0;
    uint8_t fx_val = 0;
} xm_unit_t;

typedef struct {
    uint32_t headerLength = 9;
    uint8_t type = 0; // always 0
    uint16_t numRows = 64;
    uint16_t packedPatternSize = 0;

    std::vector<std::vector<xm_unit_t>> unpk_pattern;
} xm_pattern_t;

typedef struct {
    uint32_t length = 0;
    uint32_t loopStart = 0;
    uint32_t loopLength = 0;
    uint8_t volume = 0;
    int8_t finetune = 0;
    sample_type_t type = {0, 0, 0};
    uint8_t panning = 127;
    int8_t relNoteNum = 0;
    uint8_t sampleType = 0; // 0x00 = Regular DPCM data, 0xAD = 4bit ADPCM-compressed data
    char name[22];

    std::vector<int16_t> data; // unpacked sample
} xm_sample_t;

typedef struct __attribute__((packed)) {
    uint32_t size = 263;
    char name[22] = "New Instrument";
    uint8_t type = 0; // always 0
    uint16_t numSamples = 1;

    uint8_t fill;

    // if numSamples not zero
    uint32_t sampleHeaderSize = 40;
    uint8_t sampleKeymap[96] = {0};
    env_point_t volEnv[12];
    env_point_t panEnv[12];

    uint8_t numVolPoint = 0;
    uint8_t numPanPoint = 0;
    uint8_t volSusPoint = 0;
    uint8_t volLoopStart = 0;
    uint8_t volLoopEnd = 0;
    uint8_t panSusPoint = 0;
    uint8_t panLoopStart = 0;
    uint8_t panLoopEnd = 0;

    env_type_t volType = {0, 0, 0};
    env_type_t panType = {0, 0, 0};

    uint8_t vibratoType = 0;
    uint8_t vibratoSweep = 0;
    uint8_t vibratoDepth = 0;
    uint8_t vibratoRate = 0;
    uint16_t volFadeout = 1024;

    uint8_t reserved[22];

    std::vector<xm_sample_t> sample;
    std::vector<int16_t> volEnvTable;
    std::vector<int16_t> panEnvTable;
} xm_instrument_t;

void unpack_xm_pattern(const std::vector<uint8_t>& data, std::vector<std::vector<xm_unit_t>>& unpack_data, int rows, int channels);
void pack_xm_pattern(std::vector<std::vector<xm_unit_t>>& unpack_data, std::vector<uint8_t>& packed_data, int rows, int channels);

#define FILE_OPEN_ERROR -1
#define FILE_TYPE_ERROR -2
#define FILE_READ_ERROR -3

class XMFile {
private:
    FILE *xm_file = NULL;

    char xm_file_name[256];

    xm_metadata_t metadata;
    xm_header_t header;
    std::vector<xm_pattern_t> pattern;
    std::vector<xm_instrument_t> instrument;

    int read_metadata();
    void write_metadata();
    void close_xm();
    int read_header();
    void write_header();
    void read_patterns();
    void write_patterns();
    void print_pattern(uint16_t num, int startChl, int endChl, int startRow, int endRow);
    void read_instrument();
    void write_instrument();
    void write_samples(xm_instrument_t *inst);
    void read_samples(xm_instrument_t *inst);

public:
    int open_xm(const char* filename);
    int read_all();
    int save_as(const char *filename);
};

#endif