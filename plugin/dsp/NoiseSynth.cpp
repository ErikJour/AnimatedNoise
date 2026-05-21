//
// Created by Erik Jourgensen on 5/19/26.
//

#include "NoiseSynth.h"

NoiseSynth::NoiseSynth()
{
    mSampleRate = 44100.0;
    gainSmoothed.reset(mSampleRate, 0.01f);

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
    gainSmoothed.reset(mSampleRate, 0.01f);
    const float inverseSampleRate = 1.0f / static_cast<float>(mSampleRate);
    voice.functionGenerator.mAttackMultiplier = std::exp(-inverseSampleRate * std::exp(4.5f - 0.075f * 50.0f));
    voice.functionGenerator.mReleaseMultiplier = 0.95f;
}

void NoiseSynth::render(float** outputBuffers, const int sampleCount)
{
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];
    gainSmoothed.setTargetValue(0.5f);

    for (int sample = 0; sample < sampleCount; sample++) {

        const float output = voice.render();
        const float gain = gainSmoothed.getNextValue();
        outputBufferLeft[sample] = output * gain;

        if (outputBufferRight != nullptr) {

            outputBufferRight[sample] = output * gain;
        }
    }

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
    voice.noise.setAmplitude(static_cast<float>(velocity) / 127.0f);
    voice.functionGenerator.attack();

}

void NoiseSynth::noteOn(const int note, const int velocity)
{
    startVoice(note, velocity);
    const float frequency = 440.f * std::pow(2.f, (static_cast<float>(note) - 69) / 12.f);
    voice.combFilter.excite(frequency);
}

void NoiseSynth::noteOff(const int note)
{
    if (voice.note == note) {
        voice.note = 0;
        voice.functionGenerator.release();
    }

}

