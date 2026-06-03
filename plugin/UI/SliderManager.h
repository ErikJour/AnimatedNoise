//
// Created by Erik Jourgensen on 6/3/26.
//

#ifndef ANIMATEDNOISE_SLIDERMANAGER_H
#define ANIMATEDNOISE_SLIDERMANAGER_H
#include "Scene.h"
#include "AnimatedSlider.h"
#include <juce_audio_processors/juce_audio_processors.h>



class SliderManager
{
    public:
        explicit SliderManager(Scene& scene, juce::AudioProcessorValueTreeState& apvts) : mScene(scene), mApvts(apvts) {}
        ~SliderManager() = default;

        void initializeSlider(const char* paramId);
        bool handleMouseDown(const juce::MouseEvent& event, int width, int height);
        bool handleMouseDrag(const juce::MouseEvent& event, int width, int height);
        bool handleMouseUp();
        void syncFromApvts();

    private:
        Scene&                      mScene;
        std::vector<AnimatedSlider> mSliders;
        juce::AudioProcessorValueTreeState&   mApvts;
        int                         mActiveSlider = -1;
        float                       mDragOffset   = 0.0f;
        bool                        mDragging     = false;
};


#endif //ANIMATEDNOISE_SLIDERMANAGER_H