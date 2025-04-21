#include "xm_file.h"

void unpack_xm_pattern(const std::vector<uint8_t>& data, std::vector<std::vector<xm_unit_t>>& unpack_data, int rows, int channels) {
    int index = 0;
    unpack_data.resize(channels);
    for (int c = 0; c < channels; c++) {
        unpack_data[c].resize(rows);
    }
    for (int row = 0; row < rows; ++row) {
        for (int channel = 0; channel < channels; ++channel) {
            xm_unit_t& current_unit = unpack_data[channel][row];

            uint8_t mask = data[index++];

            if (mask & 0x80) {
                current_unit.mask = mask;
                if (mask & 0x01) current_unit.note = data[index++];
                else current_unit.note = 0;
                if (mask & 0x02) current_unit.inst = data[index++];
                else current_unit.inst = 0;
                if (mask & 0x04) current_unit.vol = data[index++];
                else current_unit.vol = 0;
                if (mask & 0x08) current_unit.fx_cmd = data[index++];
                else current_unit.fx_cmd = 0;
                if (mask & 0x10) current_unit.fx_val = data[index++];
                else current_unit.fx_val = 0;
            } else {
                current_unit.mask = mask;
                current_unit.note = mask;
                current_unit.inst = data[index++];
                current_unit.vol = data[index++];
                current_unit.fx_cmd = data[index++];
                current_unit.fx_val = data[index++];
            }
        }
    }
}

void pack_xm_pattern(std::vector<std::vector<xm_unit_t>>& unpack_data, std::vector<uint8_t>& packed_data, int rows, int channels) {
    int index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int channel = 0; channel < channels; ++channel) {
            xm_unit_t& current_unit = unpack_data[channel][row];

            if (current_unit.note != 0 || current_unit.inst != 0 || current_unit.vol != 0 || current_unit.fx_cmd != 0 || current_unit.fx_val != 0) {
                packed_data.push_back(current_unit.note);
                current_unit.mask = current_unit.note;
                packed_data.push_back(current_unit.inst);
                packed_data.push_back(current_unit.vol);
                packed_data.push_back(current_unit.fx_cmd);
                packed_data.push_back(current_unit.fx_val);
            } else {
                uint8_t header = 0x80;

                if (current_unit.note != 0) header |= 0x01;
                if (current_unit.inst != 0) header |= 0x02;
                if (current_unit.vol != 0) header |= 0x04;
                if (current_unit.fx_cmd != 0) header |= 0x08;
                if (current_unit.fx_val != 0) header |= 0x10;

                packed_data.push_back(header);
                unpack_data[channel][row].mask = header;

                if (header & 0x01) packed_data.push_back(current_unit.note);
                if (header & 0x02) packed_data.push_back(current_unit.inst);
                if (header & 0x04) packed_data.push_back(current_unit.vol);
                if (header & 0x08) packed_data.push_back(current_unit.fx_cmd);
                if (header & 0x10) packed_data.push_back(current_unit.fx_val);
            }
        }
    }
}

int XMFile::read_metadata() {
    fread(metadata.id, 1, 17, xm_file);
    fread(metadata.name, 1, 20, xm_file);
    fread(&metadata.X1A, 1, 1, xm_file);
    if (metadata.X1A != 0x1A) {
        printf("Metadata Error! X1A = 0x%X\n", metadata.X1A);
        return FILE_TYPE_ERROR;
    }
    fread(metadata.trkname, 1, 20, xm_file);
    fread(&metadata.version, 2, 1, xm_file);

    printf("Metadata:\n");
    printf("ID: %.17s\n", metadata.id);
    printf("Name: %.20s\n", metadata.name);
    printf("X1A: 0x%X\n", metadata.X1A);
    printf("Tracker name: %.20s\n", metadata.trkname);
    printf("Version: 0x%X\n", metadata.version);
    printf("\n");
    return 0;
}

void XMFile::write_metadata() {
    printf("Writing metadata...\n");
    fwrite(metadata.id, 1, 17, xm_file);
    fwrite(metadata.name, 1, 20, xm_file);
    fwrite(&metadata.X1A, 1, 1, xm_file);
    fwrite(metadata.trkname, 1, 20, xm_file);
    fwrite(&metadata.version, 2, 1, xm_file);
}

int XMFile::open_xm(const char* filename) {
    xm_file = fopen(filename, "rb");
    if (xm_file == NULL) {
        return FILE_OPEN_ERROR;
    }
    
    if (read_metadata() == FILE_TYPE_ERROR) {
        close_xm();
        xm_metadata_t new_meta;
        metadata = new_meta;
        return FILE_TYPE_ERROR;
    }

    strncpy(xm_file_name, filename, sizeof(xm_file_name));
    return 0;
}

void XMFile::close_xm() {
    fclose(xm_file);
}

int XMFile::read_header() {
    if (ftell(xm_file) != 60) {
        printf("Order Error! ftell(xm_file) = 0x%X(%d)\n", ftell(xm_file), ftell(xm_file));
        return FILE_READ_ERROR;
    }

    printf("Reading Info...\n");
    fread(&header, 2, 10, xm_file);
    printf("Reading OrderTable...\n");
    header.orderTable.resize(header.size - 20);
    fread(header.orderTable.data(), 1, header.size - 20, xm_file);
    printf("Header Info:\n");
    printf("Size: %d\n", header.size);
    printf("Song length: %d\n", header.songLength);
    printf("Restart position: %d\n", header.resetVector);
    printf("Number of channels: %d\n", header.numChannels);
    printf("Number of patterns: %d\n", header.numPatterns);
    printf("Number of instruments: %d\n", header.numInstruments);
    printf("Freq mode: %s\n", header.freqMode ? "Liner" : "Amiga");
    printf("Default Tempo: %d\n", header.defaultTempo);
    printf("Default BPM: %d\n", header.defaultBPM);
    printf("Order table:\n");
    for (int i = 0; i < header.songLength; i++) {
        printf("%d ", header.orderTable[i]);
    }
    printf("\n\n");
    return 0;
}

void XMFile::write_header() {
    size_t total_size = 20;
    printf("Writing header...\n");
    size_t block_start_addr = ftell(xm_file);
    fwrite(&header, 2, 10, xm_file);
    printf("Writing orderTable...(%d Bytes)\n", header.orderTable.size());
    total_size += header.orderTable.size();
    fwrite(header.orderTable.data(), 1, header.orderTable.size(), xm_file);
    size_t block_end_addr = ftell(xm_file);
    header.size = total_size;
    fseek(xm_file, block_start_addr, SEEK_SET);
    fwrite(&header.size, 4, 1, xm_file);
    fseek(xm_file, block_end_addr, SEEK_SET);
}

void XMFile::read_patterns() {
    printf("Reading patterns...\n");
    pattern.resize(header.numPatterns);
    for (int i = 0; i < header.numPatterns; i++) {
        printf("Patterm #%d:\n", i);
        fread(&pattern[i].headerLength, 4, 1, xm_file);
        printf("Header length: %d\n", pattern[i].headerLength);
        fread(&pattern[i].type, 1, 1, xm_file);
        printf("Type: %d\n", pattern[i].type);
        fread(&pattern[i].numRows, 2, 1, xm_file);
        printf("Number of rows: %d\n", pattern[i].numRows);
        fread(&pattern[i].packedPatternSize, 2, 1, xm_file);
        printf("Pattern data size: %d\n", pattern[i].packedPatternSize);
        fseek(xm_file, 9 - pattern[i].headerLength, SEEK_CUR);
        printf("Reading pattern data...\n");
        std::vector<uint8_t> packed_pattern(pattern[i].packedPatternSize);
        fread(packed_pattern.data(), 1, pattern[i].packedPatternSize, xm_file);
        printf("Unpack pattern data...\n");
        unpack_xm_pattern(packed_pattern, pattern[i].unpk_pattern, pattern[i].numRows, header.numChannels);
        printf("\n");
    }
}

void XMFile::write_patterns() {
    printf("Writing patterns...\n");
    header.numPatterns = pattern.size();
    for (int i = 0; i < header.numPatterns; i++) {
        size_t total_size = 0;
        size_t start_addr = ftell(xm_file);
        printf("Writing patterm #%d...\n", i);
        fwrite(&pattern[i].headerLength, 4, 1, xm_file);
        total_size += 4;
        fwrite(&pattern[i].type, 1, 1, xm_file);
        total_size += 1;
        fwrite(&pattern[i].numRows, 2, 1, xm_file);
        total_size += 2;
        std::vector<uint8_t> packed_pattern;
        pack_xm_pattern(pattern[i].unpk_pattern, packed_pattern, pattern[i].numRows, header.numChannels);
        size_t packed_data_size = packed_pattern.size();
        pattern[i].packedPatternSize = packed_data_size;
        fwrite(&pattern[i].packedPatternSize, 2, 1, xm_file);
        total_size += 2;
        fwrite(packed_pattern.data(), 1, packed_data_size, xm_file);
        printf("Packed data size: %d\n", packed_data_size);
        size_t end_addr = ftell(xm_file);
        fseek(xm_file, start_addr, SEEK_SET);
        pattern[i].headerLength = total_size;
        fwrite(&pattern[i].headerLength, 4, 1, xm_file);
        fseek(xm_file, end_addr, SEEK_SET);
    }
}

void XMFile::print_pattern(uint16_t num, int startChl, int endChl, int startRow, int endRow) {
    printf("PATTERN #%d: Channel %d ~ %d, Row %d ~ %d\n", num, startChl, endChl - 1, startRow, endRow - 1);
    printf("┌────");
    for (int i = startChl; i < endChl; i++) {
        printf("─────────────────────");
    }
    printf("┐\n");

    printf("│    ");
    for (int i = startChl; i < endChl; i++) {
        printf("┌────┬──────────────┐");
    }
    printf("│\n│    ");
    for (int i = startChl; i < endChl; i++) {
        printf("│Mask│  Channel %02d  │", i);
    }
    printf("│\n├────");
    for (int i = startChl; i < endChl; i++) {
        printf("┼────┼──────────────┤");
    }
    printf("│\n");

    for (int r = startRow; r < endRow; r++) {
        printf("│ %02X ", r);
        for (int c = startChl; c < endChl; c++) {
            xm_unit_t tmp = pattern[num].unpk_pattern[c][r];
            printf("│0x%02X│", tmp.mask);
            if (HAS_NOTE(tmp.mask)) {
                char note_tmp[4];
                xm_note_to_str(tmp.note, note_tmp);
                printf("%s ", note_tmp);
                // printf("%03d ", tmp.note);
            } else {
                printf("... ");
            }

            if (HAS_INSTRUMENT(tmp.mask))
                printf("%02d ", tmp.inst);
            else
                printf(".. ");

            if (HAS_VOLUME(tmp.mask)) {
                char vol_cmd;
                uint8_t vol_cmd_val;
                parse_vol_cmd(tmp.vol, &vol_cmd, &vol_cmd_val);
                printf("%c%02d ", vol_cmd, vol_cmd_val);
            } else {
                printf("... ");
            }

            if (HAS_EFFECT_TYPE(tmp.mask)) {
                if (tmp.fx_cmd > 0xF) {
                    printf("%c", tmp.fx_cmd + 55);
                } else {
                    printf("%X", tmp.fx_cmd);
                }
            } else {
                printf(".");
            }

            if (HAS_EFFECT_PARAM(tmp.mask))
                printf("%02X│", tmp.fx_val);
            else
                printf("..│");
        }
        printf("│\n");
    }

    printf("└────");
    for (int i = startChl; i < endChl; i++) {
        printf("┴────┴──────────────┘");
    }
    printf("┘\n");
}

void XMFile::read_instrument() {
    printf("Reading instrument...\n");
    instrument.resize(header.numInstruments);
    for (int i = 0; i < header.numInstruments; i++) {
        printf("Instrument #%d\n", i);
        fread(&instrument[i], 1, 29, xm_file);
        // sizeof(xm_instrument_t);
        printf("Size: %d\n", instrument[i].size);
        printf("Name: %.22s\n", instrument[i].name);
        printf("Type: %d\n", instrument[i].type);
        if (instrument[i].numSamples == 0) {
            printf("No Sample, Skip!\n");
            continue;
        }
        printf("Number of samples: %d\n", instrument[i].numSamples);
        fread(&instrument[i].sampleHeaderSize, 1, 234, xm_file);
        printf("Sample header size: %d\n", instrument[i].sampleHeaderSize);
        printf("Sample Keymap:\n");
        for (int n = 0; n < 96; n++) {
            printf("%d ", instrument[i].sampleKeymap[n]);
        }
        printf("\n");
        printf("Envelope:\n");
        printf("Volume (%s):\n", instrument[i].volType.on ? "ON" : "OFF");
        instrument[i].volEnvTable.clear();
        if (instrument[i].numVolPoint) {
            genEnvTable(instrument[i].volEnv, instrument[i].numVolPoint, instrument[i].volEnvTable);
            printf("XXXX -> YYYY\n");
            for (int n = 0; n < instrument[i].numVolPoint; n++) {
                printf("%4d -> %4d ", instrument[i].volEnv[n].x, instrument[i].volEnv[n].y);
                if (instrument[i].volType.loop) {
                    if (n == instrument[i].volLoopStart) {
                        printf("<-LOOPSTART ");
                    }
                    if (n == instrument[i].volLoopStart) {
                        printf("<-LOOPEND ");
                    }
                }
                if (instrument[i].volType.sus) {
                    if (n == instrument[i].volSusPoint) {
                        printf("<-SUSTIAN");
                    }
                }
                printf("\n");
            }
            printf("LUT:\n");
            for (int x = 0; x < instrument[i].volEnvTable.size(); x++) {
                printf("%d ", instrument[i].volEnvTable[x]);
            }
            printf("\n");
        }
        printf("Panning (%s):\n", instrument[i].panType.on ? "ON" : "OFF");
        instrument[i].panEnvTable.clear();
        if (instrument[i].numPanPoint) {
            genEnvTable(instrument[i].panEnv, instrument[i].numPanPoint, instrument[i].panEnvTable);
            printf("XXXX -> YYYY\n");
            for (int n = 0; n < instrument[i].numVolPoint; n++) {
                printf("%4d -> %4d ", instrument[i].panEnv[n].x, instrument[i].panEnv[n].y);
                if (instrument[i].panType.loop) {
                    if (n == instrument[i].panLoopStart) {
                        printf("<-LOOPSTART ");
                    }
                    if (n == instrument[i].panLoopStart) {
                        printf("<-LOOPEND ");
                    }
                }
                if (instrument[i].panType.sus) {
                    if (n == instrument[i].panSusPoint) {
                        printf("<-SUSTIAN");
                    }
                }
                printf("\n");
            }
            printf("LUT:\n");
            for (int x = 0; x < instrument[i].panEnvTable.size(); x++) {
                printf("%d ", instrument[i].panEnvTable[x]);
            }
            printf("\n");
        }
        printf("Vibrato type: %d\n", instrument[i].vibratoType);
        printf("Vibrato sweep: %d\n", instrument[i].vibratoSweep);
        printf("Vibrato depth: %d\n", instrument[i].vibratoDepth);
        printf("Vibrato rate: %d\n", instrument[i].vibratoRate);
        printf("Volume fadeout: %d\n", instrument[i].volFadeout);
        printf("%s\n", instrument[i].reserved);
        fseek(xm_file, instrument[i].size - 263, SEEK_CUR);
        read_samples(&instrument[i]);
        printf("\n");
    }
}

void XMFile::write_instrument() {
    printf("Writing instrument...\n");
    header.numInstruments = instrument.size();
    size_t total_size = 0;
    for (int i = 0; i < header.numInstruments; i++) {
        size_t total_size = 0;
        size_t start_addr = ftell(xm_file);
        printf("Writing instrument #%d...\n", i);
        fwrite(&instrument[i], 1, 29, xm_file);
        total_size += 29;
        // sizeof(xm_instrument_t);
        if (instrument[i].numSamples == 0) {
            printf("No Sample, Skip!\n");
            continue;
        }
        printf("Number of samples: %d\n", instrument[i].numSamples);
        fwrite(&instrument[i].sampleHeaderSize, 1, 234, xm_file);
        total_size += 234;
        size_t end_addr = ftell(xm_file);
        fseek(xm_file, start_addr, SEEK_SET);
        instrument[i].size = total_size;
        fwrite(&instrument[i].size, 4, 1, xm_file);
        fseek(xm_file, end_addr, SEEK_SET);
        write_samples(&instrument[i]);
    }
}

void XMFile::write_samples(xm_instrument_t *inst) {
    printf("Writing samples...\n");
    inst->numSamples = inst->sample.size();
    for (int i = 0; i < inst->numSamples; i++) {
        printf("Writing sample#%d header\n", i);
        xm_sample_t *smp = &inst->sample[i];
        smp->length = smp->data.size();
        smp->type.sample_bit = 1;
        smp->sampleType = 0;
        uint32_t writeLength = smp->length * 2;
        uint32_t writeLoopStart = smp->loopStart * 2;
        uint32_t writeLoopLength = smp->loopLength * 2;
        fwrite(&writeLength, 4, 1, xm_file);
        fwrite(&writeLoopStart, 4, 1, xm_file);
        fwrite(&writeLoopLength, 4, 1, xm_file);
        fwrite(&smp->volume, 1, 28, xm_file);
    }
    for (int i = 0; i < inst->numSamples; i++) {
        printf("Writing sample#%d data\n", i);
        xm_sample_t *smp = &inst->sample[i];
        std::vector<int16_t> dpcm_write_buf(smp->length);
        printf("Encodeing...\n");
        encode_dpcm_16bit(smp->data.data(), dpcm_write_buf.data(), smp->length);
        printf("Writing...(%d Bytes)\n", smp->length * 2);
        fwrite(dpcm_write_buf.data(), 2, smp->length, xm_file);
    }
}

void XMFile::read_samples(xm_instrument_t *inst) {
    printf("Reading samples header...\n");
    inst->sample.resize(inst->numSamples);
    for (int i = 0; i < inst->numSamples; i++) {
        xm_sample_t *smp = &inst->sample[i];
        fread(smp, 1, 40, xm_file);
        if (smp->type.sample_bit) { // 16-bit
            smp->length /= 2;
            smp->loopStart /= 2;
            smp->loopLength /= 2;
        }
        printf("Sample #%d: %.22s\n", i, smp->name);
        printf("Length: %d Samples\n", smp->length);
        printf("Type: %s\n", (smp->sampleType == 0xAD) ? "4-bit ADPCM" : "Regular DPCM");
        printf("Width: %s\n", smp->type.sample_bit ? "16-bit" : "8-bit");
        printf("Loop: %s\n", !smp->type.loop_mode ? "No Loop" : (smp->type.loop_mode == 2 ? "Ping-Pong" : "Forward"));
        printf("Loop start: %d\n", smp->loopStart);
        printf("Loop length: %d\n", smp->loopLength);
        printf("Relative note number: %d\n", smp->relNoteNum);
        printf("Panning: %d\n", smp->panning);
    }
    printf("Reading samples data...\n");
    for (int i = 0; i < inst->numSamples; i++) {
        xm_sample_t *smp = &inst->sample[i];
        if (smp->type.sample_bit) { // 16-bit sample
            printf("#%d Reading... (16bit)\n", i);
            std::vector<int16_t> unpack_buf(smp->length);
            fread(unpack_buf.data(), 2, smp->length, xm_file);
            printf("#%d Unpacking...\n", i);
            smp->data.resize(smp->length);
            decode_dpcm_16bit(unpack_buf.data(), smp->data.data(), smp->data.size());
        } else { // 8-bit sample
            printf("#%d Reading... (8bit)\n", i);
            std::vector<int8_t> unpack_buf(smp->length);
            std::vector<int8_t> unpack_out_buf(smp->length);
            fread(unpack_buf.data(), 1, smp->length, xm_file);
            printf("#%d Unpacking...\n", i);
            smp->data.resize(smp->length);
            decode_dpcm_8bit(unpack_buf.data(), unpack_out_buf.data(), smp->length);
            for (int x = 0; x < smp->length; x++) {
                smp->data[x] = unpack_out_buf[x] << 8;
            }
        }
    }
}

int XMFile::read_all() {
    if (read_header()) {
        return FILE_READ_ERROR;
    }
    read_patterns();
    read_instrument();
    close_xm();
    return 0;
}

int XMFile::save_as(const char *filename) {
    xm_file = fopen(filename, "wb");
    write_metadata();
    write_header();
    write_patterns();
    write_instrument();
    close_xm();
    printf("Save sucess.\n");
    return 0;
}