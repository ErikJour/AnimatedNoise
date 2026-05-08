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
    const int top    = (int)(b.getHeight() * 0.375f);
    const int bottom = (int)(b.getHeight() * 0.575f);
    if (e.y >= top && e.y <= bottom)
    {
        mDragging = true;
        const int indicatorY = top + (int)((1.0f - webGpuWindow.getSliderValue()) * (bottom - top));
        mDragOffset = e.y - indicatorY;
        updateSliderFromMouse(e.y - mDragOffset);
    }
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (mDragging)
        updateSliderFromMouse(e.y - mDragOffset);
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    mDragging = false;      // ← local
}

void AudioPluginAudioProcessorEditor::updateSliderFromMouse(int screenY)
{
    auto b = getLocalBounds();
    // Derived from shader constants: juce_y = (1 - clipY) / 2 * height
    // SPINE_MAX_Y=0.25 → (1-0.25)/2 = 0.375   (v=1, top of travel)
    // SPINE_MIN_Y=-0.15 → (1+0.15)/2 = 0.575  (v=0, bottom of travel)
    const int   top    = (int)(b.getHeight() * 0.375f);
    const int   bottom = (int)(b.getHeight() * 0.575f);
    const float v      = 1.0f - juce::jlimit(0.0f, 1.0f,
        (float)(screenY - top) / (float)(bottom - top));
    webGpuWindow.setSliderValue(v);
}