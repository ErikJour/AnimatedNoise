//
// Created by Erik Jourgensen on 5/19/26.
//

#include "CombFilter.h"
CombFilter::CombFilter()
{
    mWritePosition = 0;
    mSampleRate = 44100;
}

CombFilter::~CombFilter() = default;

void CombFilter::reset(const double sampleRate, const int numChannels)
{
    mSampleRate = sampleRate;
    mRingBufferSize = static_cast<int>(mSampleRate) * 2;
    mRingBuffer.setSize(numChannels, mRingBufferSize);
}

void CombFilter::processRingBuffer(const int numSamples, const int channel, const float* channelData)
{
    if (mRingBufferSize > numSamples + mWritePosition)
    {
        mRingBuffer.copyFrom(channel,
                        mWritePosition,
                                    channelData,
                                    numSamples);
    }
    else
    {
        const auto numSamplesToEnd = mRingBufferSize - mWritePosition;
        mRingBuffer.copyFrom(channel,
                                    mWritePosition,
                                    channelData,
                                    numSamplesToEnd);

        const auto samplesLeftOver = numSamples - numSamplesToEnd;

        mRingBuffer.copyFrom(channel,
                                   0,
                                   channelData + numSamplesToEnd,
                                   samplesLeftOver);
    }
}

void CombFilter::advanceWritePosition(const int numSamples)
{
    mWritePosition += numSamples;
    mWritePosition %= mRingBufferSize;
}
