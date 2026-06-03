//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0) { }

void NoiseVoice::reset(const double sampleRate)
{
    note = 0;
    mNoiseGenerator.setLevel(0.8f);
    mSampleRate = sampleRate;
    mGain.distributeResources(mSampleRate);
    mCombFilter.reset(sampleRate);
    mFunctionGenerator.reset();
}

void NoiseVoice::render(float* buffer, const int sampleCount)
{
    mNoiseGenerator.process(buffer, sampleCount);
    mCombFilter.process(buffer, sampleCount);
    mFunctionGenerator.process(buffer, sampleCount);
    mGain.process(buffer, sampleCount);
}

void NoiseVoice::release() { mFunctionGenerator.release(); }

