#ifndef ANIMATEDNOISE_ANIMATEDSLIDER_H
#define ANIMATEDNOISE_ANIMATEDSLIDER_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <cstdint>

struct AnimatedSlider
{
    juce::ParameterID paramID;                      // source of truth for APVTS sync
    float             angle        = 0.0f;          // local angle within the room
    int               curveVariant = 0;
    std::uint32_t     materialId   = 0;             // which MAT_* slot this slider drives
    int               glowIndex    = 0;             // position in shader's sliderLevels[4]

    float             value        = 0.0f;
    bool              pressed      = false;

    std::unique_ptr<juce::ParameterAttachment> attachment;  // param→UI binding
};

#endif