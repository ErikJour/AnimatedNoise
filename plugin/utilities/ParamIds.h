//
// Created by Erik Jourgensen on 5/28/26.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================

namespace ParameterID
{
#define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

    //======================
    //Noise Parameters
    //======================
    PARAMETER_ID(noiseLevel)
    PARAMETER_ID(noiseDensity)
    PARAMETER_ID(noiseLevelMod)
    PARAMETER_ID(noiseDensityMod)
    //======================
    //LPG Parameters
    //======================
    PARAMETER_ID(lpgResonance)
    PARAMETER_ID(lpgVactrolRelease)
    //======================
    //Amp Env Parameters
    //======================
    PARAMETER_ID(envAttack)
    PARAMETER_ID(envDecay)
    PARAMETER_ID(envSustain)
    PARAMETER_ID(envRelease)
    //======================
    //Filter Parameters
    //======================
    PARAMETER_ID(coeffA)
    PARAMETER_ID(coeffB)

#undef PARAMETER_ID

}

