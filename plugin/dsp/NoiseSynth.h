//
// Created by Erik Jourgensen on 5/19/26.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "NoiseVoice.h"
#include "Utils.h"

class NoiseSynth
{
public:
    NoiseSynth();

    void distributeResources(double sampleRate, int samplesPerBlock);
    static void releaseResources();
    void reset(double sampleRate);
    void render(float** outputBuffers, int sampleCount);
    void midiMessages(uint8_t data0, uint8_t data1, uint8_t data2);
    void startVoice(int note, int velocity);
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void setGain(float gain);

private:
    double mSampleRate;
    NoiseVoice voice;


};