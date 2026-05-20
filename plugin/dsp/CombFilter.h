//
// Created by Erik Jourgensen on 5/19/26.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class CombFilter
{
public:
    CombFilter();
    ~CombFilter();

    void reset(double sampleRate, int numChannels);
    void processRingBuffer(int numSamples, int channel, const float* channelData);
    void advanceWritePosition(int numSamples);


private:
    juce::AudioBuffer<float> mRingBuffer;
    int mWritePosition;
    int mReadPosition;
    double mSampleRate;
    int mRingBufferSize = 0.0;

};


