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
    const auto height       = static_cast<float>(getHeight());
    const auto width        = static_cast<float>(getWidth());
    const float top         = webGpuWindow.getScene().sliderTopFraction()    * height;
    const float bottom      = webGpuWindow.getScene().sliderBottomFraction() * height;
    const float sliderX     = webGpuWindow.getScene().sliderXFraction()      * width;
    constexpr float hitRadius = 20.0f;

    if (const float halfIndicator = Scene::indicatorHalfFraction() * height;
        static_cast<float>(e.y) >= top - halfIndicator &&
        static_cast<float>(e.y) <= bottom + halfIndicator &&
        std::abs(static_cast<float>(e.x) - sliderX) <= hitRadius)
    {
        const float indicatorY = bottom - webGpuWindow.getScene().getSliderValue() * (bottom - top);
        mDragOffset = static_cast<float>(e.y) - indicatorY;
        mDragging   = true;
        updateSliderFromMouse(e.y);
    }
    else
    {
        // Miss on slider — begin camera drag
        mCameraDragging = true;
        mLastMouseX     = static_cast<float>(e.x);
        mLastMouseY     = static_cast<float>(e.y);
        webGpuWindow.getScene().onMouseButton(0, true,
                                       static_cast<float>(e.x),
                                       static_cast<float>(e.y));
    }
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (mDragging)
    {
        updateSliderFromMouse(e.y);
        return;
    }

    if (mCameraDragging)
    {
        const auto x = static_cast<float>(e.x);
        const auto y = static_cast<float>(e.y);
        webGpuWindow.getScene().onMouseMove(x, y);
        mLastMouseX = x;
        mLastMouseY = y;
    }
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    mDragging       = false;
    mDragOffset     = 0.0f;

    if (mCameraDragging)
    {
        webGpuWindow.getScene().onMouseButton(0, true,
                                      static_cast<float>(e.x),
                                      static_cast<float>(e.y));
        mCameraDragging = false;
    }
}

void AudioPluginAudioProcessorEditor::updateSliderFromMouse(const int screenY)
{
    const auto height      = static_cast<float>(getHeight());
    const float top        = webGpuWindow.getScene().sliderTopFraction()    * height;
    const float bottom     = webGpuWindow.getScene().sliderBottomFraction() * height;

    float v = (bottom - (static_cast<float>(screenY) - mDragOffset)) / (bottom - top);
    v = juce::jlimit(0.0f, 1.0f, v);
    webGpuWindow.getScene().setSliderValue(v);
}

void AudioPluginAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
                                                      const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    webGpuWindow.getScene().onScroll(wheel.deltaY);
}