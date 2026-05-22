//
// Created by Erik Jourgensen on 5/19/26.
//


#pragma once
#include "NoiseGenerator.h"
#include "CombFilter.h"
#include "FunctionGenerator.h"
#include <juce_audio_processors/juce_audio_processors.h>

class NoiseVoice {

    public:

    NoiseVoice();
    ~NoiseVoice() = default;

    void reset(double sampleRate);
    float render();
    void release();

    NoiseGenerator noise;
    int note = 0;
    int noiseType = 0;
    CombFilter combFilter;
    FunctionGenerator functionGenerator;
    juce::AudioBuffer<float> mAudioBuffer;
    double mSampleRate;


};