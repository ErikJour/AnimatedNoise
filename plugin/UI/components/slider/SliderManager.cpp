#include "SliderManager.h"
#include "sliderCatalog.h"
#include <iostream>
#include <cfloat>

void SliderManager::initializeSliders()
{
    const auto& defs = sliderDefinitions();
    mSliders.clear();
    mSliders.reserve(defs.size());

    for (const auto& def : defs)
    {
        auto& s        = mSliders.emplace_back();
        s.paramID      = def.paramID;
        s.angle        = def.angle;
        s.curveVariant = def.curveVariant;
        s.materialId   = def.materialId;
        s.glowIndex    = def.glowIndex;

        auto* param = mApvts.getParameter(def.paramID.getParamID());
        jassert(param != nullptr);

        s.attachment = std::make_unique<juce::ParameterAttachment>(
            *param,
            [&s](float newValue) { s.value = newValue; });

        s.attachment->sendInitialUpdate();
    }

    mScene.setSliderList(mSliders);
}

bool SliderManager::handleMouseDown(const juce::MouseEvent& event, int width, int height)
{
    const auto screenW               = static_cast<float>(width);
    const auto screenH               = static_cast<float>(height);
    const juce::Point<float> mouse{ (float)event.x, (float)event.y };
    constexpr float kHitRadius       = 24.0f;   // distance to the tube segment
    constexpr float kIndicatorRadius = 28.0f;   // radial grab around the bead
    constexpr float kIndicatorHalfH  = 0.048f;  // must match shader halfH (shadeSpineTube)


     int   bestIndex = -1;
     float bestDepth  = FLT_MAX;
     float bestTRaw  = 0.0f;

    for (auto& s : mSliders)
    {
        juce::Point<float> top, bottom;
        mScene.projectSliderBounds(screenW, screenH, top, bottom, s.angle);

        const juce::Point<float> axis = top - bottom;
        const float axisLen2 = axis.x * axis.x + axis.y * axis.y;

        const float tRaw  = (mouse - bottom).getDotProduct(axis) / axisLen2;
        const float tClmp = juce::jlimit(0.0f, 1.0f, tRaw);

        const float beadV = juce::jlimit(kIndicatorHalfH, 1.0f - kIndicatorHalfH, s.value);

        const juce::Point<float> bead    = mScene.projectSliderPoint(screenW, screenH, beadV, s.angle);
        float hitDepth = mScene.getDepthValue();
        const juce::Point<float> nearest = mScene.projectSliderPoint(screenW, screenH, tClmp, s.angle);

        const bool onIndicator = mouse.getDistanceFrom(bead)    <= kIndicatorRadius;
        const bool onTube      = mouse.getDistanceFrom(nearest) <= kHitRadius;


        if ((onIndicator || onTube) && hitDepth < bestDepth)
        {
            bestDepth = hitDepth;
            bestIndex = static_cast<int>(&s - mSliders.data());
            bestTRaw = tRaw;
        }
    }
    if (bestIndex < 0) return false;
    auto& s = mSliders[static_cast<size_t>(bestIndex)];
    mActiveSlider = bestIndex;
    mDragOffsetT  = bestTRaw - s.value;
    mDragging     = true;
    s.pressed     = true;

    const float v = juce::jlimit(0.0f, 1.0f, bestTRaw - mDragOffsetT);
    s.attachment->beginGesture();
    s.attachment->setValueAsPartOfGesture(v);
    return true;
}

bool SliderManager::handleMouseDrag(const juce::MouseEvent& event, int width, int height) const
{
    if (!mDragging) return false;
    auto& s = mSliders[static_cast<size_t>(mActiveSlider)];

    juce::Point<float> top, bottom;
    mScene.projectSliderBounds(static_cast<float>(width), static_cast<float>(height),
                               top, bottom, s.angle);

    const juce::Point<float> axis = top - bottom;
    const float axisLen2 = axis.x * axis.x + axis.y * axis.y;
    const juce::Point<float> mouse{ (float)event.x, (float)event.y };

    float t = (mouse - bottom).getDotProduct(axis) / axisLen2;
    const float v = juce::jlimit(0.0f, 1.0f, t - mDragOffsetT);
    s.attachment->setValueAsPartOfGesture(v);
    return true;
}

bool SliderManager::handleMouseUp()
{
    if (mActiveSlider >= 0)
    {
        auto& s = mSliders[static_cast<size_t>(mActiveSlider)];
        s.pressed = false;
        s.attachment->endGesture();
    }
    mDragging     = false;
    mDragOffsetT  = 0.0f;
    mActiveSlider = -1;
    return true;
}