//
// Created by Erik Jourgensen on 5/19/26.
//


#pragma once
#include "NoiseGenerator.h"
#include "CombFilter.h"
#include "FunctionGenerator.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "AnimatedGain.h"
#include  "AnimatedLPG.h"
#include "AnimatedVactrol.h"

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
    FunctionGenerator mFunctionGenerator;
    AnimatedGain mGain;
    juce::AudioBuffer<float> mAudioBuffer;
    double mSampleRate;

    //low pass gate
    AnimatedLPG mLPG;
    float rfSmoothed = 1e4f;
    float rfTarget = 5e4f;
    float rfSmoothedCoeff = 0.0050f;
    float rfDecayMult = 1.05f;
    AnimatedVactrol mVactrol;

};