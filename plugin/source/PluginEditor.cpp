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
    processorRef.savedCameraState = webGpuWindow.getScene().getCameraState();
    webGpuWindow.terminate();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();

    if (!webGpuWindow.hasSurface() && getPeer() != nullptr) {
        if (void* handle = getWindowHandle()) {
            const auto width = static_cast<uint32_t>(getWidth());
            const auto height = static_cast<uint32_t>(getHeight());
            webGpuWindow.initSurface(handle, width, height);
            mConfiguredW = width;
            mConfiguredH = height;
            webGpuWindow.getScene().setCameraState(processorRef.savedCameraState);
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
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    float centerX, topY, bottomY;
    webGpuWindow.getScene().projectSliderBounds(w, h, centerX, topY, bottomY);

    constexpr float kHitRadiusX = 24.0f;
    const float     ey          = static_cast<float>(e.y);
    const float     ex          = static_cast<float>(e.x);

    if (ey >= topY && ey <= bottomY && std::abs(ex - centerX) <= kHitRadiusX)
    {
        const float indicatorY = bottomY - webGpuWindow.getScene().getSliderValue()
                                           * (bottomY - topY);
        mDragOffset = ey - indicatorY;
        mDragging   = true;
        updateSliderFromMouse(e.y);
    }
    else
    {
        mCameraDragging = true;
        mLastMouseX     = ex;
        mLastMouseY     = ey;
        webGpuWindow.getScene().onMouseButton(0, true, ex, ey);
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
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    float centerX, topY, bottomY;
    webGpuWindow.getScene().projectSliderBounds(w, h, centerX, topY, bottomY);

    float v = (bottomY - (static_cast<float>(screenY) - mDragOffset)) / (bottomY - topY);
    v = juce::jlimit(0.0f, 1.0f, v);
    webGpuWindow.getScene().setSliderValue(v);
}

void AudioPluginAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
                                                      const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    webGpuWindow.getScene().onScroll(wheel.deltaX, wheel.deltaY);
}