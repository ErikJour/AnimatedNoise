//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0)
{
}

void NoiseVoice::reset(const double sampleRate)
{
    note = 0;
    noise.setLevel(0.8f);
    mSampleRate = sampleRate;
    combFilter.reset(sampleRate);
}

float NoiseVoice::render()
{
    const float whiteNoiseSample = noise.getNextSample();
    const float noiseX = whiteNoiseSample;
    const float output = combFilter.process(noiseX);
    return output;
}

