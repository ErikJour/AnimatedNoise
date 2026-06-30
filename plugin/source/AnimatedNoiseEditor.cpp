#include "AnimatedNoiseProcessor.h"
#include "AnimatedNoiseEditor.h"

//==============================================================================
AnimatedNoiseProcessorEditor::AnimatedNoiseProcessorEditor (AnimatedNoiseProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p),
                                mSliderManager(webGpuWindow.getScene(), processorRef.apvts)
{
    juce::ignoreUnused(processorRef);

    constexpr int initWidth  = 800;
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

    if (webGpuWindow.hasSurface() || getPeer() == nullptr)
        return;

    const auto*  primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    const double scale = primaryDisplay ? primaryDisplay->scale : 1.0;

    const auto   width  = static_cast<uint32_t>(std::round(static_cast<double>(getWidth())  * scale));
    const auto   height = static_cast<uint32_t>(std::round(static_cast<double>(getHeight()) * scale));

    if (!webGpuWindow.initSurface(scale, width, height))
        return;

#if JUCE_MAC
    mMetalView.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mMetalView);
    mMetalView.setView(webGpuWindow.getNativeView());
    mMetalView.setBounds(getLocalBounds());
#endif

    mStartTimeMs  = juce::Time::getMillisecondCounterHiRes();
    mStartTimeSet = true;
    mConfiguredW  = width;
    mConfiguredH  = height;

    webGpuWindow.getScene().setCameraState(processorRef.savedCameraState);
    mSliderManager.initializeSliders();
    startTimerHz(60);

    juce::Component::SafePointer<AnimatedNoiseProcessorEditor> safeThis (this);
    juce::MessageManager::callAsync([safeThis]()
    {
        if (safeThis == nullptr) return;

        safeThis->setResizable(true, true);
        safeThis->setResizeLimits(200, 113, 3200, 1800);

        if (auto* constrainer = safeThis->getConstrainer())
            constrainer->setFixedAspectRatio(800.0 / 450.0);
    });
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

    if (mStartTimeSet)
    {
        const double elapsed = (juce::Time::getMillisecondCounterHiRes() - mStartTimeMs) * 0.001;
        webGpuWindow.getScene().renderFrame(static_cast<float>(elapsed));
    }
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
    const float scale          = primaryDisplay ? static_cast<float>(primaryDisplay->scale) : 1.0f;
    mPendingW = static_cast<uint32_t>(std::round(static_cast<float>(getWidth())  * scale));
    mPendingH = static_cast<uint32_t>(std::round(static_cast<float>(getHeight()) * scale));
    mResizePending = true;
}
//==============================================================================
void AnimatedNoiseProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto currentX = static_cast<float>(e.x);
    const auto currentY = static_cast<float>(e.y);

    if (!mSliderManager.handleMouseDown(e, getWidth(), getHeight()))
    {
        mCameraDragging = true;
        mLastMouseX     = currentX;
        mLastMouseY     = currentY;
        webGpuWindow.getScene().onMouseButton(0, true, currentX, currentY);
    }
}

void AnimatedNoiseProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!mSliderManager.handleMouseDrag(e, getWidth(), getHeight()))
    {
        if (mCameraDragging)
        {
            const auto currentX = static_cast<float>(e.x);
            const auto currentY = static_cast<float>(e.y);

            webGpuWindow.getScene().onMouseMove(currentX, currentY);
            mLastMouseX = currentX;
            mLastMouseY = currentY;
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
