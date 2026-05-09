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
    static auto startTime = juce::Time::getMillisecondCounterHiRes();
    const double elapsed  = (juce::Time::getMillisecondCounterHiRes() - startTime) * 0.001;
    webGpuWindow.renderFrame(static_cast<float>(elapsed));
}

//==============================================================================
void AudioPluginAudioProcessorEditor::resized()
{
    webGpuWindow.onResize(static_cast<uint32_t>(getWidth()),
                          static_cast<uint32_t>(getHeight()));
}

//==============================================================================
void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    auto b = getLocalBounds();
    const int top       = (int)(b.getHeight() * 0.375f);   // NDC  0.25 → screen
    const int bottom    = (int)(b.getHeight() * 0.575f);   // NDC -0.15 → screen
    const int sliderX   = (int)(b.getWidth()  * 0.75f);    // NDC  0.50 → screen
    const int hitRadius = 20;                                // ≈ 2× indicator half-width in px

    if (e.y >= top && e.y <= bottom &&
        e.x >= sliderX - hitRadius && e.x <= sliderX + hitRadius)
    {
        mDragging = true;
        updateSliderFromMouse(e.y);
    }
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (mDragging)
        updateSliderFromMouse(e.y);
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    mDragging = false;
}

void AudioPluginAudioProcessorEditor::updateSliderFromMouse(int screenY)
{
    auto b = getLocalBounds().toFloat();
    const float top    = b.getHeight() * 0.375f;
    const float bottom = b.getHeight() * 0.575f;

    float v = (bottom - (float)screenY) / (bottom - top);
    v = juce::jlimit(0.0f, 1.0f, v);

    webGpuWindow.setSliderValue(v);
}