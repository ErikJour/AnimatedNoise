#ifndef ANIMATEDNOISE_SLIDERCATALOG_H
#define ANIMATEDNOISE_SLIDERCATALOG_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdint>
#include <vector>
#include "../../../shaders/MyUniforms.h"


struct SliderDef
{
    juce::ParameterID paramID;       // APVTS parameter this slider drives
    float             angle;         // local angle within that room
    std::uint32_t     materialId;    // MAT_* uniform / material slot
};


const std::vector<SliderDef>& sliderDefinitions();


#endif // ANIMATEDNOISE_SLIDERCATALOG_H
