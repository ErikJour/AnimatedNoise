//
// Created by Erik Jourgensen on 5/19/26.
//

#include "NoiseGenerator.h"


NoiseGenerator::NoiseGenerator() : mSampleRate(44100), noiseLevel(0.0f), mAmplitude(0.0f)
{
    levelSmoothed.reset(mSampleRate, 0.02f);
    densitySmoothed.reset(mSampleRate, 0.05f);
    densitySmoothed.setCurrentAndTargetValue(1.0f);
}


float NoiseGenerator::getNextSample()
{
    noiseLevel = levelSmoothed.getNextValue();
    const float currentDensity = densitySmoothed.getNextValue();

    if (currentDensity >= 0.999f)
    {
        const float output = (random.nextFloat() * 2.0f - 1.0f) * noiseLevel * mAmplitude;
        constexpr float reduction = 0.2f;
        return output * reduction;
    }

    if (currentDensity <= 0.001f) { return 0.0f; }

    if (mHoldCounter <= 0)
    {
        const int maxHold = static_cast<int>(20.0f * (1.0f - currentDensity)) + 1;
        mHoldCounter = random.nextInt(maxHold);

        if (random.nextFloat() < currentDensity)
        {
            mCurrentNoiseValue = (random.nextFloat() * 2.0f - 1.0f);
        }
        else
        {
            mCurrentNoiseValue = 0.0f;
        }
    }

    mHoldCounter--;

    const float output = mCurrentNoiseValue * noiseLevel * mAmplitude;

    constexpr float reduction = 0.2f;
    return output * reduction;
}

void NoiseGenerator::setDensity(float newDensity)
{
    juce::ignoreUnused(newDensity);
    // newDensity *= noiseDensityMod;
    // newDensity = juce::jlimit(0.0f,
    //                                     1.0f,
    //                                     newDensity);
    densitySmoothed.setTargetValue(newDensity);
}

void NoiseGenerator::setSampleRate(const float newSampleRate)
{
    mSampleRate = newSampleRate;
    levelSmoothed.reset(mSampleRate, 0.02f);
    densitySmoothed.reset(mSampleRate, 0.05f);
}

void NoiseGenerator::process(float* buffer, const int numSamples)
{
    for (int i = 0; i < numSamples; i++)
        buffer[i] += getNextSample();

}

