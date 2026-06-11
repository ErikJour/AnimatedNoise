//
// Created by Erik Jourgensen on 6/3/26.
//

#include "SliderManager.h"

struct SliderDefinition {
    const char* paramId;
    float       angle;
};

constexpr SliderDefinition kSliderDefinitions[] = {
    { "noiseLevel",   0.0f   },
    { "noiseDensity", 0.1f   },
    { "lpgResonance", 1.45f  },
    { "combLevel",    2.975f }
};

void SliderManager::initializeSliders()
{
    for (const auto& [paramId, angle] : kSliderDefinitions)
        mSliders.push_back(AnimatedSlider{ angle, paramId, 0.0f });
    syncFromApvts();
}

bool SliderManager::handleMouseDown(const juce::MouseEvent& event, const int width, const int height)
{
    const auto screenWidth      = static_cast<float>(width);
    const auto screenHeight     = static_cast<float>(height);
    const auto     ey           = static_cast<float>(event.y);
    const auto     ex           = static_cast<float>(event.x);
    constexpr float kHitRadiusX = 24.0f;

    for (size_t i = 0; i < mSliders.size(); ++i)
    {
        float centerX, topY, bottomY;

        mScene.projectSliderBounds(screenWidth,
                                                screenHeight,
                                                centerX,
                                                topY,
                                                bottomY,
                                                mSliders[i].angle);

        if (ey >= topY && ey <= bottomY && std::abs(ex - centerX) <= kHitRadiusX)
        {
            mActiveSlider = static_cast<int>(i);
            mDragOffset   = ey - (bottomY - mSliders[i].value * (bottomY - topY));
            mDragging     = true;

            float v = (bottomY - (ey - mDragOffset)) / (bottomY - topY);
            v = juce::jlimit(0.0f, 1.0f, v);
            mSliders[i].value = v;
            mScene.setSliderValue(mActiveSlider, v);

            if (auto* param = mApvts.getParameter(mSliders[i].paramID))
                param->setValueNotifyingHost(v);

            return true;
        }
    }
    return false;
}
bool SliderManager::handleMouseDrag(const juce::MouseEvent& event, const int width, const int height)
{
    if (!mDragging) return false;

    const auto w = static_cast<float>(width);
    const auto h = static_cast<float>(height);
    auto& slider  = mSliders[static_cast<size_t>(mActiveSlider)];

    float centerX, topY, bottomY;

    mScene.projectSliderBounds(w, h, centerX, topY, bottomY, slider.angle);

    float v = (bottomY - (static_cast<float>(event.y) - mDragOffset)) / (bottomY - topY);
    v = juce::jlimit(0.0f, 1.0f, v);
    slider.value = v;
    mScene.setSliderValue(mActiveSlider, v);

    if (auto* param = mApvts.getParameter(slider.paramID))
        param->setValueNotifyingHost(v);
    return true;
}

bool SliderManager::handleMouseUp()
{
    mDragging       = false;
    mDragOffset     = 0.0f;
    return true;
}

void SliderManager::syncFromApvts()
{
    for (size_t i = 0; i < mSliders.size(); ++i)
    {
        auto& slider = mSliders[i];
        if (const auto* param = mApvts.getParameter(slider.paramID))
        {
            const float v = param->getValue();
            if (std::abs(v - slider.value) > 0.001f)
            {
                slider.value = v;
                mScene.setSliderValue(static_cast<int>(i), v);
            }
        }
    }
}
