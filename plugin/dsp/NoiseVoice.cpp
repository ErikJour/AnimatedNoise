//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0) { }

void NoiseVoice::reset(const double sampleRate)
{
    note = 0;
    mNoiseGenerator.setLevel(0.5f);
    mSampleRate = sampleRate;
    mGain.distributeResources(mSampleRate);
    mCombFilter.reset(sampleRate);
    mFunctionGenerator.reset();
    mLPG.prepare(mSampleRate);
    mLPG.setMode(AnimatedLPG::Mode::LowPass);
    mLPG.setResonance(0.5f);
    mVactrol.prepare(mSampleRate);
}

void NoiseVoice::render(float* buffer, const int sampleCount)
{
    mNoiseGenerator.process(buffer, sampleCount);
    mCombFilter.process(buffer, sampleCount);
    mLPG.processBufferModulated(buffer, sampleCount, [this]{ return mVactrol.tick(); });
    mFunctionGenerator.process(buffer, sampleCount);
    mGain.process(buffer, sampleCount);
}

void NoiseVoice::release() { mFunctionGenerator.release(); }

