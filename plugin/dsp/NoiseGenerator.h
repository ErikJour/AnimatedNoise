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
    void setCombLevel(const float newCombLevel) {combSmoothed.setTargetValue(newCombLevel);}
    void setDensity(float newDensity);
    void setSampleRate(float newSampleRate);
    void setNoiseDensityModAmt(const float noiseDensityModAmt) { noiseDensityMod = noiseDensityModAmt; }
    void process(float* buffer, int numSamples);

private:

juce::Random random;
double mSampleRate;
float noiseLevel;
float mAmplitude;
float mCombLevel{};

float mCurrentNoiseValue{};
int mHoldCounter{};
juce::SmoothedValue<float> levelSmoothed;
juce::SmoothedValue<float> densitySmoothed;
juce::SmoothedValue<float> combSmoothed;

float noiseDensityMod{};


//==============================================================================

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGenerator)
};