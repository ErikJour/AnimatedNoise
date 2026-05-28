//
// Created by Erik Jourgensen on 5/19/26.
//


#pragma once
#include "NoiseGenerator.h"
#include "CombFilter.h"
#include "FunctionGenerator.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "AnimatedGain.h"

class NoiseVoice {

    public:

    NoiseVoice();
    ~NoiseVoice() = default;

    void reset(double sampleRate);
    void render(float* buffer, int sampleCount);
    void release();

    NoiseGenerator mNoiseGenerator;
    int note = 0;
    int noiseType = 0;
    CombFilter mCombFilter;
    FunctionGenerator mFunctionGenerator;
    AnimatedGain mGain;
    juce::AudioBuffer<float> mAudioBuffer;
    double mSampleRate;


};