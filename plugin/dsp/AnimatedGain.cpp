//
// Created by Erik Jourgensen on 5/28/26.
//

#include "AnimatedGain.h"

AnimatedGain::AnimatedGain() : mSampleRate(44100.0) {}

AnimatedGain::~AnimatedGain() {}

void AnimatedGain::distributeResources(const double sampleRate)
{
    mSampleRate = sampleRate;
    gainSmoothed.reset(mSampleRate, 0.01f);
    gainSmoothed.setCurrentAndTargetValue(1.0f);

}

void AnimatedGain::setGain(const float newGain)
{

    //Map values of 0 - 1 to a target value that corresponds to decibels
    const float dB = juce::jmap(newGain,
                        0.0f,
                        1.0f,
                        -24.0f,
                        24.0f);

    // Convert dB to linear gain
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(dB, -24.0f));
}

float AnimatedGain::getGain() const { return gainSmoothed.getTargetValue(); }

void AnimatedGain::process(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        buffer[i] *= gainSmoothed.getNextValue();
    }

}