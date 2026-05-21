//
// Created by Erik Jourgensen on 5/19/26.
//

#include "CombFilter.h"


CombFilter::CombFilter()
{
    mSampleRate = 44100;
    mDecay = 0.99f;
}

CombFilter::~CombFilter() = default;

void CombFilter::reset(const double sampleRate)
{
    mSampleRate = sampleRate;
}

void CombFilter::excite(const float frequency)
{
    ringBufferLength = static_cast<uint32_t>(mSampleRate / frequency);
    if (ringBufferLength >= MAX_BUFFER_LENGTH) {
        ringBufferLength = MAX_BUFFER_LENGTH - 1;
    }

    for (uint32_t i = 0; i < ringBufferLength; i++)
        ringBufferMemory[i] = 0.f;

    ringBufferIndex = 0;

    mPrevSample = 0.f;
}


float CombFilter::process(const float input)
{
    const float delayed  = ringBufferMemory[ringBufferIndex];
    const float filtered = 0.5f * (delayed + mPrevSample);
    // filtered = (delayed * (1.0 - damping)) + (mPrevSample * damping); // add mDamping with range 0 to 0.99

    ringBufferMemory[ringBufferIndex] = (filtered + input) * mDecay;

    ringBufferIndex++;

    if (ringBufferIndex >= ringBufferLength)
        ringBufferIndex = 0;

    mPrevSample = delayed;
    return filtered;
}

