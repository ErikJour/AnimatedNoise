//
// Created by Erik Jourgensen on 5/19/26.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <random>

class NoiseGenerator
{
public:

    NoiseGenerator();
    ~NoiseGenerator() = default;

    float getNextSample();
    void setAmplitude(const float newAmplitude) { mAmplitude = newAmplitude; }
    void setLevel(const float newNoiseLevel) {levelSmoothed.setTargetValue(newNoiseLevel);}
    void setDensity(float newDensity);
    void setSampleRate(float newSampleRate);
    void setNoiseDensityModAmt(const float noiseDensityModAmt) { noiseDensityMod = noiseDensityModAmt; }
    void process(float* buffer, int numSamples);


private:

juce::Random random;
double mSampleRate;
float noiseLevel;
float mAmplitude;

float mCurrentNoiseValue{};
int mHoldCounter{};
juce::SmoothedValue<float> levelSmoothed;
juce::SmoothedValue<float> densitySmoothed;
float noiseDensityMod{};


//==============================================================================

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGenerator)
};