//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0)
{
}

void NoiseVoice::reset(const double sampleRate, const int numChannels)
{
    note = 0;
    noise.setLevel(0.8f);
    mSampleRate = sampleRate;
    combFilter.reset(sampleRate, numChannels);
}

float NoiseVoice::render()
{
    const float whiteNoiseSample = noise.getNextSample();
    const float output = whiteNoiseSample;
    return output;
    DBG("Output" << output);
}

