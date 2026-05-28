//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0) { }

void NoiseVoice::reset(const double sampleRate)
{
    note = 0;
    noise.setLevel(0.8f);
    mSampleRate = sampleRate;
    mGain.distributeResources(mSampleRate);
    combFilter.reset(sampleRate);
    functionGenerator.reset();
}

void NoiseVoice::render(float* buffer, const int sampleCount)
{
    functionGenerator.process(buffer, sampleCount);
    combFilter.process(buffer, sampleCount);
    mGain.process(buffer, sampleCount);
}

void NoiseVoice::release() { functionGenerator.release(); }

