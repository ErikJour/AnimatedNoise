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
            const auto w = static_cast<uint32_t>(getWidth());
            const auto h = static_cast<uint32_t>(getHeight());
            webGpuWindow.initSurface(handle, w, h);
            mConfiguredW = w;
            mConfiguredH = h;
            startTimerHz(60);
            juce::MessageManager::callAsync([this]() {
                setResizable(true, true);
                setResizeLimits(200, 113, 3200, 1800);
                getConstrainer()->setFixedAspectRatio(800.0 / 450.0);
            });
        }
    }
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    if (mResizePending) {
        if (mPendingW != mConfiguredW || mPendingH != mConfiguredH) {
            webGpuWindow.onResize(mPendingW, mPendingH);
            mConfiguredW = mPendingW;
            mConfiguredH = mPendingH;
        }
        mResizePending = false;
    }

    static auto startTime = juce::Time::getMillisecondCounterHiRes();
    const double elapsed  = (juce::Time::getMillisecondCounterHiRes() - startTime) * 0.001;
    webGpuWindow.getScene().renderFrame(static_cast<float>(elapsed));
}

void AudioPluginAudioProcessorEditor::setResizeReady()
{
    setResizable(true, true);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::resized()
{
    if (!webGpuWindow.hasSurface()) return;
    mPendingW = static_cast<uint32_t>(getWidth());
    mPendingH = static_cast<uint32_t>(getHeight());
    mResizePending = true;
}

//==============================================================================
void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto h       = static_cast<float>(getHeight());
    const auto w       = static_cast<float>(getWidth());
    const float top     = webGpuWindow.getScene().sliderTopFraction()    * h;
    const float bottom  = webGpuWindow.getScene().sliderBottomFraction() * h;
    const float sliderX = webGpuWindow.getScene().sliderXFraction()      * w;
    constexpr float hitRadius = 20.0f;

    if (const float halfIndicator = Scene::indicatorHalfFraction() * h; static_cast<float>(e.y) >= top - halfIndicator && static_cast<float>(e.y) <= bottom + halfIndicator &&
        std::abs(static_cast<float>(e.x) - sliderX) <= hitRadius)
    {
        const float indicatorY = bottom - webGpuWindow.getScene().getSliderValue() * (bottom - top);
        mDragOffset = static_cast<float>(e.y) - indicatorY;
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

void AudioPluginAudioProcessorEditor::updateSliderFromMouse(const int screenY)
{
    const auto h      = static_cast<float>(getHeight());
    const float top    = webGpuWindow.getScene().sliderTopFraction()    * h;
    const float bottom = webGpuWindow.getScene().sliderBottomFraction() * h;

    float v = (bottom - (static_cast<float>(screenY) - mDragOffset)) / (bottom - top);
    v = juce::jlimit(0.0f, 1.0f, v);
    webGpuWindow.getScene().setSliderValue(v);
}

void AudioPluginAudioProcessorEditor::onMouseMove(double xpos, double ypos)
{
    juce::ignoreUnused(xpos, ypos);

}
void AudioPluginAudioProcessorEditor::onMouseButton(int button, int action, int mods)
{
    juce::ignoreUnused(button, action, mods);

}
void AudioPluginAudioProcessorEditor::onScroll(double xoffset, double yoffset)
{
    juce::ignoreUnused(xoffset, yoffset);

}