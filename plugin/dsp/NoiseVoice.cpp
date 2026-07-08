//
// Created by Erik Jourgensen on 5/19/26.
//
#include "NoiseVoice.h"

NoiseVoice::NoiseVoice() : mSampleRate(0) { }

void NoiseVoice::reset(const double sampleRate)
{
    mSampleRate     = sampleRate;
    note            = 0;
    mNoiseGenerator.setLevel(0.5f);
    mLPG.prepare(mSampleRate);
    mLPG.setMode(AnimatedLPG::Mode::LowPass);
    mLPG.setResonance(0.9f);
    mVactrol.prepare(mSampleRate);
    mEnvelope.reset();
    mGain.distributeResources(mSampleRate);

}

void NoiseVoice::render(float* buffer, const int sampleCount)
{
    mNoiseGenerator.process(buffer, sampleCount);
    mLPG.processBufferModulated(buffer, sampleCount, [this]{ return mVactrol.tick(); });
    mEnvelope.process(buffer, sampleCount);
}

void NoiseVoice::release() { mEnvelope.release(); }

