//
// Created by Erik Jourgensen on 5/28/26.
//

#ifndef ANIMATEDNOISE_ANIMATEDGAIN_H
#define ANIMATEDNOISE_ANIMATEDGAIN_H
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class AnimatedGain
{
public:
    AnimatedGain();
    ~AnimatedGain();

    void distributeResources(double sampleRate);
    void setGain(float newGain);
    [[nodiscard]] float getGain() const;
    void process(float* buffer, int numSamples);

private:
    double mSampleRate;
    juce::SmoothedValue<float> gainSmoothed;

};






#endif //ANIMATEDNOISE_ANIMATEDGAIN_H