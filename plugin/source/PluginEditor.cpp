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
    const float h       = (float)getHeight();
    const float w       = (float)getWidth();
    const float top     = webGpuWindow.sliderTopFraction()    * h;
    const float bottom  = webGpuWindow.sliderBottomFraction() * h;
    const float sliderX = webGpuWindow.sliderXFraction()      * w;
    constexpr float hitRadius = 20.0f;

    const float halfIndicator = webGpuWindow.indicatorHalfFraction() * h;
    if ((float)e.y >= top - halfIndicator && (float)e.y <= bottom + halfIndicator &&
        std::abs((float)e.x - sliderX) <= hitRadius)
    {
        const float indicatorY = bottom - webGpuWindow.getSliderValue() * (bottom - top);
        mDragOffset = (float)e.y - indicatorY;
        mDragging   = true;
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
    mDragging   = false;
    mDragOffset = 0.0f;
}

void AudioPluginAudioProcessorEditor::updateSliderFromMouse(int screenY)
{
    const float h      = (float)getHeight();
    const float top    = webGpuWindow.sliderTopFraction()    * h;
    const float bottom = webGpuWindow.sliderBottomFraction() * h;

    float v = (bottom - ((float)screenY - mDragOffset)) / (bottom - top);
    v = juce::jlimit(0.0f, 1.0f, v);
    webGpuWindow.setSliderValue(v);
}