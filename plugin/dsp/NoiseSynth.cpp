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
    voice.mVactrol.strike(static_cast<float>(velocity) / midiScaling);
    Envelope& env = voice.mEnvelope;
    env.attackMultiplier  = envAttack;
    env.decayMultiplier   = envDecay;
    env.releaseMultiplier = envRelease;
    env.attack();
}

void NoiseSynth::noteOn(const int note, const int velocity)
{
    startVoice(note, velocity);
}

void NoiseSynth::noteOff(const int note)
{
    if (voice.note == note) {
        voice.note = 0;
        voice.mEnvelope.release();
        voice.mVactrol.releaseGate();
    }
}



