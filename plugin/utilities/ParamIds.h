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
    PARAMETER_ID(combLevel)
    PARAMETER_ID(lpgResonance)

#undef PARAMETER_ID

}

