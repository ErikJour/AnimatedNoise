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
        if (void* handle = getWindowHandle()) {
            const auto width = static_cast<uint32_t>(getWidth());
            const auto height = static_cast<uint32_t>(getHeight());
            webGpuWindow.initSurface(handle, width, height);
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
    const double elapsed  = (juce::Time::getMillisecondCounterHiRes() - startTime) * 0.001;
    webGpuWindow.getScene().renderFrame(static_cast<float>(elapsed));

    syncParameters();

}

void AnimatedNoiseProcessorEditor::setResizeReady()
{
    setResizable(true, true);
}

//==============================================================================
void AnimatedNoiseProcessorEditor::resized()
{
    if (!webGpuWindow.hasSurface()) return;
    mPendingW = static_cast<uint32_t>(getWidth());
    mPendingH = static_cast<uint32_t>(getHeight());
    mResizePending = true;
}

//==============================================================================
void AnimatedNoiseProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!mSliderManager.handleMouseDown(event, getWidth(), getHeight()))
    {
        mCameraDragging = true;
        mLastMouseX     = static_cast<float>(event.x);
        mLastMouseY     = static_cast<float>(event.y);
        webGpuWindow.getScene().onMouseButton(0,
                                    true,
                                        static_cast<float>(event.x),
                                        static_cast<float>(event.y));
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

void AnimatedNoiseProcessorEditor::syncParameters()
{
    mSliderManager.syncFromApvts();
}