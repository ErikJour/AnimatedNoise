#ifndef ANIMATEDNOISE_ANIMATEDSLIDER_H
#define ANIMATEDNOISE_ANIMATEDSLIDER_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <cstdint>

struct AnimatedSlider
{
    juce::ParameterID paramID;
    float             angle        = 0.0f;
    std::uint32_t     materialId   = 0;
    float             value        = 0.0f;
    bool              pressed      = false;


    std::unique_ptr<juce::ParameterAttachment> attachment;  // param→UI binding

};

#endif