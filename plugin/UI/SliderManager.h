#ifndef ANIMATEDNOISE_SLIDERMANAGER_H
#define ANIMATEDNOISE_SLIDERMANAGER_H
#include "Scene.h"
#include "AnimatedSlider.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class SliderManager
{
public:
    explicit SliderManager(Scene& scene, juce::AudioProcessorValueTreeState& apvts)
        : mScene(scene), mApvts(apvts) {}
    ~SliderManager() = default;

    void initializeSliders();
    bool handleMouseDown(const juce::MouseEvent& event, int width, int height);
    bool handleMouseDrag(const juce::MouseEvent& event, int width, int height) const;
    bool handleMouseUp();

    const std::vector<AnimatedSlider>& sliders() const { return mSliders; }

private:
    Scene&                                mScene;
    std::vector<AnimatedSlider>           mSliders;
    juce::AudioProcessorValueTreeState&   mApvts;
    int                                   mActiveSlider = -1;
    float mDragOffsetT = 0.0f;
    bool                                  mDragging     = false;
};

#endif