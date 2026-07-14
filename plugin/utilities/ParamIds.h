//
// Created by Erik Jourgensen on 5/28/26.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================

namespace ParameterID
{
#define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

    PARAMETER_ID(noiseLevel)
    PARAMETER_ID(noiseDensity)
    PARAMETER_ID(lpgResonance)
    PARAMETER_ID(lpgVactrolRelease)
    PARAMETER_ID(envAttack)
    PARAMETER_ID(envDecay)
    PARAMETER_ID(envSustain)
    PARAMETER_ID(envRelease)
    PARAMETER_ID(gain)

#undef PARAMETER_ID

}

