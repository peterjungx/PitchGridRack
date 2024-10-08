#pragma once
#include <rack.hpp>
#include "pitchgrid.hpp"

union {
    uint8_t b[4];
    float f;
    unsigned int i;
} typedef FloatUnion;


struct DataLink {
    FloatUnion START_MESSAGE = {{0x54, 0x44, 0x41, 0x06}};
    FloatUnion END_MESSAGE = {{0x54, 0x44, 0x41, 0x0D}};
    std::vector<FloatUnion> dataRed;
    std::vector<FloatUnion> dataBlue;
    unsigned int num_values = 0;
    unsigned int state = 0;
    bool processingRed = true;
    
};

struct DataSender: DataLink {
    DataSender();
    void addFloatValue(float value);
    void setFloatValue(unsigned int index, float value);
    void addIntValue(int value);
    void setIntValue(unsigned int index, int value);
    void processWithOutput(rack::engine::Output* output);
};

struct DataReceiver: DataLink {
    DataReceiver();
    void addValue();
    float getFloatValue(unsigned int index);
    int getIntValue(unsigned int index);
    void processWithInput(rack::engine::Input* input);
};

struct TuningDataSender: DataSender {
    TuningDataSender();
    void addTuningData(ConsistentTuning* tuning, RegularScale* scale);
    void setTuningData(ConsistentTuning* tuning, RegularScale* scale);
};

struct TuningDataReceiver: DataReceiver {
    TuningDataReceiver();
    void initialize();
    void getTuningData(ConsistentTuning* tuning, RegularScale* scale);
};

