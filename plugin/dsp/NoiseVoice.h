//
// Created by Erik Jourgensen on 5/19/26.
//


#pragma once
#include "NoiseGenerator.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include  "AnimatedLPG.h"
#include "AnimatedVactrol.h"
#include "Envelope.h"
#include "CombFilter.h"

struct NoiseVoice {
    NoiseVoice();
    ~NoiseVoice() = default;

    void reset(double sampleRate);
    void render(float* buffer, int sampleCount);
    void release();

    NoiseGenerator  mNoiseGenerator;
    Envelope        mEnvelope;
    AnimatedVactrol mVactrol;
    CombFilter      mCombFilter;
    AnimatedLPG     mLPG;

    int note      = 0;
    int noiseType = 0;
    juce::AudioBuffer<float> mAudioBuffer;
    double mSampleRate;

};