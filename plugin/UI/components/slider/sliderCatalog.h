#ifndef ANIMATEDNOISE_SLIDERCATALOG_H
#define ANIMATEDNOISE_SLIDERCATALOG_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdint>
#include <vector>
#include "../../../shaders/MyUniforms.h"

inline constexpr float kSliderRadius = 0.06f;

struct SliderDef
{
    juce::ParameterID paramID;
    float             angle;
    std::uint32_t     materialId;
    float             position[3];
    float             radius;
};


const std::vector<SliderDef>& sliderDefinitions();


#endif // ANIMATEDNOISE_SLIDERCATALOG_H
