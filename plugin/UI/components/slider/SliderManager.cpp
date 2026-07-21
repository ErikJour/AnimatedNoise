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
        auto& s      = mSliders.emplace_back();
        s.paramID    = def.paramID;
        s.angle      = def.angle;
        s.materialId = def.materialId;

        auto* param = mApvts.getParameter(def.paramID.getParamID());

        jassert(param != nullptr);

        s.attachment = std::make_unique<juce::ParameterAttachment>(
            *param,
            [&s](const float newValue) { s.value = newValue; });

        s.attachment->sendInitialUpdate();
    }

    mScene.setSliderList(mSliders);
}

bool SliderManager::handleMouseDown(const juce::MouseEvent& event, const int width, const int height)
{
    //Raycasting setup
    const float x = (2.0f * static_cast<float>(event.x)) / static_cast<float>(width) - 1.0f;
    const float y = 1.0f - (2.0f * static_cast<float>(event.y)) / static_cast<float>(height);

    const float ray_clip[4] = { -x, y, -1.0f, 1.0f };

    float ray_eye[4];
    mulMat4Vec4(ray_eye, mScene.invProj(), ray_clip);
    ray_eye[2] = -1.0f;
    ray_eye[3] =  0.0f;

    float ray_wor[4];
    mulMat4Vec4(ray_wor, mScene.invView(), ray_eye);
    const float len = std::sqrt(ray_wor[0]*ray_wor[0]
                              + ray_wor[1]*ray_wor[1]
                              + ray_wor[2]*ray_wor[2]);
    ray_wor[0] /= len;  ray_wor[1] /= len;  ray_wor[2] /= len;

    const float* iv = mScene.invView();
    const float rayOrigin[3] = { iv[12], iv[13], iv[14] };

    const auto& sliders = sliderDefinitions();
    int bestIndex = -1;
    float bestT = FLT_MAX;


    for (int i = 0; i < static_cast<int>(sliders.size()); ++i)
    {
        const float* c = sliders[static_cast<size_t>(i)].position;
        const float oc[3] = { rayOrigin[0]-c[0], rayOrigin[1]-c[1], rayOrigin[2]-c[2] };
        const float b  = oc[0]*ray_wor[0] + oc[1]*ray_wor[1] + oc[2]*ray_wor[2];
        const float cc = oc[0]*oc[0] + oc[1]*oc[1] + oc[2]*oc[2] - kSliderRadius*kSliderRadius;
        const float disc = b*b - cc;

        if (disc < 0.0f) continue;
        const float t = -b - std::sqrt(disc);
        if (t > 0.0f && t < bestT) { bestT = t; bestIndex = i; }
    }

    if (bestIndex < 0) return false;
    auto& s     = mSliders[static_cast<size_t>(bestIndex)];
    mDragStartValue          = s.value;
    mDragStartY              = event.y;
    mActiveSlider            = bestIndex;
    mDragging                = true;
    s.pressed                = true;
    mDragOffsetT             = 0.0f;
    s.attachment->beginGesture();
    sendParamStringTooltip(s.value);
    return true;
}

bool SliderManager::handleMouseDrag(const juce::MouseEvent& event, int, int) const
{
    if (!mDragging) return false;

    constexpr float kPixelsForFullRange       = 100.0f;
    const float delta                         = static_cast<float>(mDragStartY - event.y) / kPixelsForFullRange;
    const float v                             = juce::jlimit(0.0f, 1.0f, mDragStartValue + delta);
    auto& s                 = mSliders[static_cast<size_t>(mActiveSlider)];
    const auto* param = mApvts.getParameter(s.paramID.getParamID());

    s.attachment->setValueAsPartOfGesture(param->convertFrom0to1(v));
    sendParamStringTooltip(v);
    return true;
}

bool SliderManager::handleMouseUp()
{
    if (mActiveSlider >= 0)
    {
        auto& s = mSliders[static_cast<size_t>(mActiveSlider)];
        s.pressed = false;
        s.attachment->endGesture();
        mScene.setToolTip("", "");

    }
    mDragging     = false;
    mDragOffsetT  = 0.0f;
    mActiveSlider = -1;
    return true;
}

void SliderManager::sendParamStringTooltip(const float value) const
{
    if (mActiveSlider >= 0)
    {
        auto& s               = mSliders[static_cast<size_t>(mActiveSlider)];
        const int intVal                        = static_cast<int>(value * 100);
        std::string paramString                 = s.paramID.getParamID().toStdString();
        const juce::String paramValue           = std::to_string(intVal);
        if (paramString == "noiseDensity")      {paramString = "Noise Density"; }
        if (paramString == "noiseLevel")        {paramString = "Noise Level"; }
        const std::string paramValueString      = paramValue.toStdString() + "%";

        mScene.setToolTip(paramString, paramValueString);
    }
}




