#include "AnimatedNoiseProcessor.h"
#include "AnimatedNoiseEditor.h"

//==============================================================================
AnimatedNoiseProcessorEditor::AnimatedNoiseProcessorEditor (AnimatedNoiseProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), mSliderManager(webGpuWindow.getScene(), processorRef.apvts)
{
    juce::ignoreUnused(processorRef);
    constexpr int initWidth = 800;
    constexpr int initHeight = 450;

    setSize(initWidth, initHeight);

    webGpuWindow.initialize();
}

AnimatedNoiseProcessorEditor::~AnimatedNoiseProcessorEditor()
{
    stopTimer();
    processorRef.savedCameraState = webGpuWindow.getScene().getCameraState();
    webGpuWindow.terminate();
}

//==============================================================================
void AnimatedNoiseProcessorEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();

    if (!webGpuWindow.hasSurface() && getPeer() != nullptr) {
        const auto* primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        const double scale = primaryDisplay ? primaryDisplay->scale : 1.0;
        const auto width  = static_cast<uint32_t>(std::round(static_cast<double>(getWidth())  * scale));
        const auto height = static_cast<uint32_t>(std::round(static_cast<double>(getHeight()) * scale));

        if (!webGpuWindow.initSurface(scale, width, height))
            return;

#if JUCE_MAC
        // The Metal layer lives in its own NSView, confined to this editor, so
        // it never paints over the standalone window's title bar.
        // The native view's -hitTest: returns nil so AppKit hands mouse events to
        // JUCE's peer; this makes JUCE's own routing skip the overlay component too,
        // so mouseDown reaches the editor (slider/text hit-testing) instead of dying
        // in the NSViewComponent. (mouseWheel already bubbles to the parent.)
        mMetalView.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(mMetalView);
        mMetalView.setView(webGpuWindow.getNativeView());
        mMetalView.setBounds(getLocalBounds());
#endif

        mStartTimeMs = juce::Time::getMillisecondCounterHiRes();
        mStartTimeSet = true;
        mConfiguredW = width;
        mConfiguredH = height;
        webGpuWindow.getScene().setCameraState(processorRef.savedCameraState);
        mSliderManager.initializeSliders();
        startTimerHz(60);
        juce::MessageManager::callAsync([this]() {
            setResizable(true, true);
            setResizeLimits(200,
                            113,
                            3200,
                            1800);
            getConstrainer()->setFixedAspectRatio(800.0 / 450.0);
        });
    }
}

void AnimatedNoiseProcessorEditor::timerCallback()
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
    const double elapsed        = (juce::Time::getMillisecondCounterHiRes() - startTime) * 0.001;

    webGpuWindow.getScene().renderFrame(static_cast<float>(elapsed));

    // syncParameters();
}

void AnimatedNoiseProcessorEditor::setResizeReady()
{
    setResizable(true, true);
}

//==============================================================================
void AnimatedNoiseProcessorEditor::resized()
{
    if (!webGpuWindow.hasSurface()) return;
#if JUCE_MAC
    mMetalView.setBounds(getLocalBounds());
#endif
    const auto* primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    const float scale = primaryDisplay ? static_cast<float>(primaryDisplay->scale) : 1.0f;
    mPendingW = static_cast<uint32_t>(std::round(static_cast<float>(getWidth())  * scale));
    mPendingH = static_cast<uint32_t>(std::round(static_cast<float>(getHeight()) * scale));
    mResizePending = true;
}

//==============================================================================
void AnimatedNoiseProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    const auto fx = static_cast<float>(event.x);
    const auto fy = static_cast<float>(event.y);

    if (!mSliderManager.handleMouseDown(event, getWidth(), getHeight()))
    {
        mCameraDragging = true;
        mLastMouseX     = fx;
        mLastMouseY     = fy;
        webGpuWindow.getScene().onMouseButton(0, true, fx, fy);
    }
}

void AnimatedNoiseProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (!mSliderManager.handleMouseDrag(event, getWidth(), getHeight()))
    {
        if (mCameraDragging)
        {
            const auto x = static_cast<float>(event.x);
            const auto y = static_cast<float>(event.y);
            webGpuWindow.getScene().onMouseMove(x, y);
            mLastMouseX = x;
            mLastMouseY = y;
        }
    }
}

void AnimatedNoiseProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    mSliderManager.handleMouseUp();

    if (mCameraDragging)
    {
        webGpuWindow.getScene().onMouseButton(0, true,
                                      static_cast<float>(e.x),
                                      static_cast<float>(e.y));
        mCameraDragging = false;
    }
}

void AnimatedNoiseProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
                                                      const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    webGpuWindow.getScene().onScroll(wheel.deltaX, wheel.deltaY);
}
