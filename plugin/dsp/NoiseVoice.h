//
// Created by Erik Jourgensen on 5/19/26.
//


#pragma once
#include "NoiseGenerator.h"

struct NoiseVoice
{
    NoiseGenerator noise;
    int note;
    int noiseType;


void reset()
{
    note = 0;
    noise.setLevel(0.8f);
}

float render()
{
    const float whiteNoiseSample = noise.getNextSample();
    const float output = whiteNoiseSample;
    return output;
}

};