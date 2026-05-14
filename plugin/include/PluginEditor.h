#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "utilityHelper.h"
#include "webGpuWindow.h"
#include "Scene.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
    void mouseDown   (const juce::MouseEvent& e) override;
    void mouseDrag   (const juce::MouseEvent& e) override;
    void mouseUp     (const juce::MouseEvent& e) override;
    void updateSliderFromMouse(int screenY);
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    void paint(juce::Graphics&) override {}
private:
    void parentHierarchyChanged() override;
    void timerCallback() override;

    AudioPluginAudioProcessor& processorRef;
    WebGpuWindow webGpuWindow;
    bool  mDragging    = false;
    float mDragOffset  = 0.0f;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
