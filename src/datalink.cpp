#pragma once
#include <rack.hpp>
#include "datalink.hpp"

    //rack::engine::Output* output;
    DataSender::DataSender(){};
    //DataSender(rack::engine::Output* output): output(output) {};
    void DataSender::addFloatValue(float value){
        FloatUnion u1;
        u1.f = value;
        dataBlue.push_back(u1);
        FloatUnion u2;
        u2.f = value;
        dataRed.push_back(u2);
        num_values++;
    };
    void DataSender::setFloatValue(unsigned int index, float value){
        if (processingRed){
            dataBlue[index].f = value;
        } else {
            dataRed[index].f = value;
        }
    };
    void DataSender::addIntValue(int value){
        FloatUnion u1;
        u1.i = value;
        dataBlue.push_back(u1);
        FloatUnion u2;
        u2.i = value;
        dataRed.push_back(u2);
        num_values++;
    };
    void DataSender::setIntValue(unsigned int index, int value){
        if (processingRed){
            dataBlue[index].i = value;
        } else {
            dataRed[index].i = value;
        }
    };
    void DataSender::processWithOutput(rack::engine::Output* output){
        if (!output)return;
        if (state == 0){
            output->setVoltage(START_MESSAGE.f);
            state = 1;
        }else if (state < num_values + 1){
            if (processingRed){
                output->setVoltage(dataRed[state-1].f);
            } else {
                output->setVoltage(dataBlue[state-1].f);
            }
            state++;
        }else if (state == num_values + 1){
            output->setVoltage(END_MESSAGE.f);
            state = 0;
            processingRed = !processingRed;
        }
    };


    //rack::engine::Input* input;
    //DataReceiver(rack::engine::Input* input): input(input) {};
    DataReceiver::DataReceiver(){};
    void DataReceiver::addValue(){
        FloatUnion u1;
        u1.i = 0;
        dataBlue.push_back(u1);

        FloatUnion u2;
        u2.i = 0;
        dataRed.push_back(u2);
        num_values++;
    };
    float DataReceiver::getFloatValue(unsigned int index){
        if (processingRed){
            return dataBlue[index].f;
        } else {
            return dataRed[index].f;
        }
    };
    int DataReceiver::getIntValue(unsigned int index){
        if (processingRed){
            return dataBlue[index].i;
        } else {
            return dataRed[index].i;
        }
    };
    void DataReceiver::processWithInput(rack::engine::Input* input){
        if (!input)return;
        FloatUnion u;
        u.f = input->getVoltage();
        if (state == 0){
            if (u.f == START_MESSAGE.f){
                state = 1;
            }
        }else if (state < num_values + 1){
            if (processingRed){
                dataRed[state-1].f = u.f;
            } else {
                dataBlue[state-1].f = u.f;
            }
            state++;
        }else if (state == num_values + 1){
            if (u.f == END_MESSAGE.f){
                state = 0;
                processingRed = !processingRed;
            }
        }
    };




    TuningDataSender::TuningDataSender(){};
    //TuningDataSender(rack::engine::Output* output): DataSender(output) {};
    void TuningDataSender::addTuningData(ConsistentTuning* tuning, RegularScale* scale){
        addIntValue(tuning->V1().x);
        addIntValue(tuning->V1().y);
        addFloatValue(tuning->F1());
        addIntValue(tuning->V2().x);
        addIntValue(tuning->V2().y);
        addFloatValue(tuning->F2());
        addIntValue(scale->scale_class.x);
        addIntValue(scale->scale_class.y);
        addIntValue(scale->mode);
    };
    void TuningDataSender::setTuningData(ConsistentTuning* tuning, RegularScale* scale){
        ScaleVector v = tuning->V1();
        setIntValue(0, v.x);
        setIntValue(1, v.y);
        setFloatValue(2, tuning->F1());
        v = tuning->V2();
        setIntValue(3, v.x);
        setIntValue(4, v.y);
        setFloatValue(5, tuning->F2());
        setIntValue(6, scale->scale_class.x);
        setIntValue(7, scale->scale_class.y);
        setIntValue(8, scale->mode);
    };


    TuningDataReceiver::TuningDataReceiver(){};
    void TuningDataReceiver::initialize(){
        for (int i=0; i<9; i++){
            addValue();
        }
    }
    void TuningDataReceiver::getTuningData(ConsistentTuning* tuning, RegularScale* scale){
        tuning->setParams({getIntValue(0), getIntValue(1)}, getFloatValue(2), {getIntValue(3), getIntValue(4)}, getFloatValue(5));
        scale->setScaleClass({getIntValue(6), getIntValue(7)});
        scale->mode = getIntValue(8);
    };

    