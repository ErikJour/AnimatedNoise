//
// Created by Erik Jourgensen on 5/19/26.
//

#include "NoiseSynth.h"

NoiseSynth::NoiseSynth()
{
    mSampleRate = 44100.0;
}

void NoiseSynth::distributeResources(const double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    juce::ignoreUnused(samplesPerBlock);
}

void NoiseSynth::releaseResources() {}

void NoiseSynth::reset(const double sampleRate)
{
    mSampleRate = sampleRate;
    voice.reset(mSampleRate);
    const float inverseSampleRate = 1.0f / static_cast<float>(mSampleRate);
    voice.mFunctionGenerator.mAttackMultiplier  = std::exp(-inverseSampleRate * std::exp(4.5f - 0.075f * 50.0f));
    voice.mFunctionGenerator.mReleaseMultiplier = std::exp(-inverseSampleRate / 1.5f); // 1.5 second release
    voice.mVactrol.setReleaseTime(1.0f);
}

void NoiseSynth::render(float** outputBuffers, const int sampleCount)
{
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];

    voice.render(outputBufferLeft, sampleCount);
    if (outputBufferRight != nullptr)
        juce::FloatVectorOperations::copy(outputBufferRight, outputBufferLeft, sampleCount);

    protectMyEars(outputBufferLeft, sampleCount);
    protectMyEars(outputBufferRight, sampleCount);
}

void NoiseSynth::midiMessages(uint8_t data0, uint8_t data1, uint8_t data2)
{
    switch (data0 & 0xF0){
    case 0x80:
        noteOff(data1 & 0x7f);
        break;
    case 0x90:
        const uint8_t note = data1 & 0x7F;
        uint8_t velocity = data2 & 0x7F;
        if (velocity > 0) {
            noteOn(note, velocity);
        }
        else {
            noteOff(note);
        }
        break;
    }
}

void NoiseSynth::startVoice(const int note, const int velocity)
{
    voice.note = note;
    constexpr float midiScaling = 127.0f;
    voice.mNoiseGenerator.setAmplitude(static_cast<float>(velocity) / midiScaling);
    voice.mFunctionGenerator.attack();
    voice.mCombFilter.setAmplitude(static_cast<float>(velocity) / midiScaling);
    voice.mVactrol.strike(static_cast<float>(velocity) / midiScaling);
}

void NoiseSynth::noteOn(const int note, const int velocity)
{
    startVoice(note, velocity);
    const float frequency = 440.f * std::pow(2.f, (static_cast<float>(note) - 69) / 12.0f);
    voice.mCombFilter.excite(frequency);
}

void NoiseSynth::noteOff(const int note)
{
    if (voice.note == note) {
        voice.note = 0;
        voice.mFunctionGenerator.release();
        voice.mVactrol.releaseGate();
    }
}



