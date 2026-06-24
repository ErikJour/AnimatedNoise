#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "AnimatedNoiseProcessor.h"
#include "utilityHelper.h"
#include "webGpuWindow.h"
#include "SliderManager.h"
#include "Scene.h"

//==============================================================================
class AnimatedNoiseProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit AnimatedNoiseProcessorEditor (AnimatedNoiseProcessor&);
    ~AnimatedNoiseProcessorEditor() override;

    //==============================================================================
    void resized() override;
    void mouseDown   (const juce::MouseEvent& e) override;
    void mouseDrag   (const juce::MouseEvent& e) override;
    void mouseUp     (const juce::MouseEvent& e) override;
    void paint(juce::Graphics&) override {}
private:
    void parentHierarchyChanged() override;
    void timerCallback() override;
    void setResizeReady();
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void syncParameters();

    AnimatedNoiseProcessor& processorRef;
    WebGpuWindow webGpuWindow;
    bool  mDragging    = false;
    float mDragOffset  = 0.0f;
    int mActiveSlider = 0;

    bool     timerReady     = false;
    bool     mResizePending = false;
    uint32_t mPendingW      = 0;
    uint32_t mPendingH      = 0;
    uint32_t mConfiguredW   = 0;
    uint32_t mConfiguredH   = 0;

    //Camera
    bool  mCameraDragging = false;
    float mLastMouseX     = 0.0f;
    float mLastMouseY     = 0.0f;

    //Sliders
    SliderManager mSliderManager;

#if JUCE_MAC
    juce::NSViewComponent mMetalView;
#endif

    double mStartTimeMs = 0.0;
    double mLastFrameMs = 0.0;
    double mElapsed     = 0.0;
    bool   mStartTimeSet = false;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedNoiseProcessorEditor)
};
