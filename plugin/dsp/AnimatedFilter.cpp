//
// Created by Erik Jourgensen on 8/6/26.
//

#include "AnimatedFilter.h"

AnimatedFilter::AnimatedFilter() {}
AnimatedFilter::~AnimatedFilter() {}

void AnimatedFilter::reset()
{
    mDelay  = 0.0f;
    mCoeffA = 0.05f;
    mCoeffB = -0.9f;
}

float AnimatedFilter::onePoleIIR(const float input, const float delay, const float coeffA, const float coeffB)
{
    return coeffA * input - coeffB * delay;
}

void AnimatedFilter::processBuffer(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        const float input  = buffer[i];

        const float output = onePoleIIR(input, mDelay, mCoeffA, mCoeffB);
        mDelay             = output;
        buffer[i]          = output;
    }
}
