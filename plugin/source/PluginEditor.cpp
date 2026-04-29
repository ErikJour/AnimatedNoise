#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (800, 450);
    webGpuWindow.initialize();
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
    webGpuWindow.terminate();
}

//==============================================================================
// Called when this component gains a native peer (i.e. when the DAW shows the
// plugin window). getWindowHandle() is only valid once a peer exists.
void AudioPluginAudioProcessorEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();

    if (!webGpuWindow.hasSurface() && getPeer() != nullptr) {
        if (void* handle = getWindowHandle()) {
            webGpuWindow.initSurface(handle,
                                     static_cast<uint32_t>(getWidth()),
                                     static_cast<uint32_t>(getHeight()));
            startTimerHz(60);
        }
    }
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    webGpuWindow.renderFrame();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebGPU owns the entire background — JUCE painting is intentionally skipped.
    // If the surface hasn't been created yet, the window is blank until the first frame.
}

void AudioPluginAudioProcessorEditor::resized()
{
    webGpuWindow.onResize(static_cast<uint32_t>(getWidth()),
                          static_cast<uint32_t>(getHeight()));
}
