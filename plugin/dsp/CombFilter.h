//
// Created by Erik Jourgensen on 5/19/26.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <random>

//We can control level and damping and can modulate both

#define MAX_BUFFER_LENGTH 100000

class CombFilter
{
public:
    CombFilter();
    ~CombFilter();

    void reset(double sampleRate);
    void excite(float frequency);
    void process(float* buffer, int numSamples);
    void setLevel(float newCombLevel);
    void setAmplitude(const float newAmplitude) { mAmplitude = newAmplitude; }
    float updateLevel();
    float processSample(float input);
    void setDamping(float newDampingLevel);
    float updateDamping();

private:
    float                      ringBufferMemory[MAX_BUFFER_LENGTH] = {};
    double                     mSampleRate      = { 0.0f };
    juce::SmoothedValue<float> levelSmoothed    = { 0.0f };
    juce::SmoothedValue<float> mDampingSmoothed = { 0.0f };
    float                      mDecay           = { 0.996f };
    float                      mPrevSample      = { 0.0f };
    float                      mAmplitude       = { 0.0f };
    uint32_t                   ringBufferLength = { 0 };
    uint32_t                   ringBufferIndex  = { 0 };
};


