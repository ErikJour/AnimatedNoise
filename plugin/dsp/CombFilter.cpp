//
// Created by Erik Jourgensen on 5/19/26.
//

#include "CombFilter.h"


CombFilter::CombFilter()
{
    mSampleRate = 44100;
    mDecay = 0.99f;
    levelSmoothed.reset(mSampleRate, 0.02f);

}

CombFilter::~CombFilter() = default;

void CombFilter::reset(const double sampleRate) { mSampleRate = sampleRate; }

void CombFilter::excite(const float frequency)
{
    ringBufferLength = static_cast<uint32_t>(mSampleRate / frequency);
    if (ringBufferLength >= MAX_BUFFER_LENGTH)
        ringBufferLength = MAX_BUFFER_LENGTH - 1;

    // Seed with noise instead of zeros — this IS the pluck
    for (uint32_t i = 0; i < ringBufferLength; i++)
        ringBufferMemory[i] = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;;

    ringBufferIndex = 0;
    mPrevSample = 0.f;
}

float CombFilter::processSample(const float input)
{
    const float combLevel = updateLevel();
    const float delayed  = ringBufferMemory[ringBufferIndex];
    const float filtered = 0.7f * delayed + 0.3f * mPrevSample;

    // filtered = (delayed * (1.0 - damping)) + (mPrevSample * damping); // add mDamping with range 0 to 0.99

    ringBufferMemory[ringBufferIndex] = (filtered + input) * mDecay;

    ringBufferIndex++;

    if (ringBufferIndex >= ringBufferLength)
        ringBufferIndex = 0;

    mPrevSample = delayed;
    return filtered * combLevel * mAmplitude;
}

void CombFilter::process(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
        buffer[i] += processSample(0.0f);
}

void CombFilter::setLevel(const float newCombLevel)
{
    levelSmoothed.setTargetValue(newCombLevel);
}

float CombFilter::updateLevel()
{
    return levelSmoothed.getNextValue();
}



