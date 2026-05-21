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
    functionGenerator.reset();
}

float NoiseVoice::render()
{
    const float functionGeneratorSample = functionGenerator.nextValue();
    float output = combFilter.process(0.0f);
    output *= functionGeneratorSample;
    return output;
}

void NoiseVoice::release()
{
    functionGenerator.release();
}

