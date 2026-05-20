//
// Created by Erik Jourgensen on 5/19/26.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <random>

#define MAX_BUFFER_LENGTH 100000

class CombFilter
{
public:
    CombFilter();
    ~CombFilter();

    void reset(double sampleRate);
    void excite(float frequency);
    float process(float input);

private:
    double mSampleRate { 0.0f };
    //New

    uint32_t ringBufferLength = { 0 };
    float ringBufferMemory[MAX_BUFFER_LENGTH] = {};
    uint32_t ringBufferIndex = { 0 };

    float mDecay      = 0.996f; // controls sustain
    float mPrevSample = 0.f;
};


