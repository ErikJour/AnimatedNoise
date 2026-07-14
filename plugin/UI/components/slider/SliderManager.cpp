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
    //Step 1 - Normalize coordinates -1 to 1
    const float x = (2.0f * static_cast<float>(event.x)) / static_cast<float>(width) - 1.0f;
    std::cout << "X is: " << x << std::endl;

    const float y = 1.0f - (2.0f * static_cast<float>(event.y)) / static_cast<float>(height);
    std::cout << "Y is: " << y << std::endl;

    //Step 2 - clip coordinates
    const float ray_clip[4] = { -x, y, -1.0f, 1.0f };

    //Step 3 - Camera coordinates
    float ray_eye[4];
    mulMat4Vec4(ray_eye, mScene.invProj(), ray_clip);
    ray_eye[2] = -1.0f;
    ray_eye[3] =  0.0f;

    //Step 4 - World Coordinates
    float ray_wor[4];
    mulMat4Vec4(ray_wor, mScene.invView(), ray_eye);
    const float len = std::sqrt(ray_wor[0]*ray_wor[0]
                              + ray_wor[1]*ray_wor[1]
                              + ray_wor[2]*ray_wor[2]);
    ray_wor[0] /= len;  ray_wor[1] /= len;  ray_wor[2] /= len;

    std::cout << "ray_wor: ("
          << ray_wor[0] << ", "
          << ray_wor[1] << ", "
          << ray_wor[2] << ")" << std::endl;

    const float* iv = mScene.invView();
    const float rayOrigin[3] = { iv[12], iv[13], iv[14] };
    //Need slider positions somehow

    // TODO ray picking, per slider:
    //   1. intersect (rayOrigin, ray_wor) against slider's world-space bounds
    //   2. keep smallest positive t across sliders  -> bestIndex
    //   3. recover position along slider axis       -> bestTRaw

    const auto& defs = sliderDefinitions();
    int bestIndex = -1;
    float bestT = FLT_MAX;

    std::cout << "rayOrigin: (" << rayOrigin[0] << ", "
              << rayOrigin[1] << ", " << rayOrigin[2] << ")\n";

    for (int i = 0; i < (int)defs.size(); ++i)
    {
        const float* c = defs[(size_t)i].position;
        const float oc[3] = { rayOrigin[0]-c[0], rayOrigin[1]-c[1], rayOrigin[2]-c[2] };
        const float b  = oc[0]*ray_wor[0] + oc[1]*ray_wor[1] + oc[2]*ray_wor[2];
        const float cc = oc[0]*oc[0] + oc[1]*oc[1] + oc[2]*oc[2] - kSliderRadius*kSliderRadius;
        const float disc = b*b - cc;

        std::cout << "slider " << i << " b=" << b << " cc=" << cc
                  << " disc=" << disc << std::endl;

        if (disc < 0.0f) continue;
        const float t = -b - std::sqrt(disc);
        if (t > 0.0f && t < bestT) { bestT = t; bestIndex = i; }
    }

    std::cout << (bestIndex >= 0 ? "HIT " : "MISS ") << bestIndex
              << " t=" << bestT << std::endl;



    if (bestIndex < 0) return false;
    auto& s = mSliders[static_cast<size_t>(bestIndex)];
    mActiveSlider = bestIndex;
    mDragging = true;
    s.pressed = true;
    mDragOffsetT  = 0.0f;
    s.attachment->beginGesture();
    return true;
}

bool SliderManager::handleMouseDrag(const juce::MouseEvent& /*event*/, int /*width*/, int /*height*/)
{
    // if (!mDragging) return false;
    // auto& s = mSliders[static_cast<size_t>(mActiveSlider)];
    //
    // juce::Point<float> top, bottom;
    // mScene.projectSliderBounds(static_cast<float>(width), static_cast<float>(height),
    //                            top, bottom, s.angle);
    //
    // const juce::Point<float> axis = top - bottom;
    // const float axisLen2 = axis.x * axis.x + axis.y * axis.y;
    // const juce::Point<float> mouse{ (float)event.x, (float)event.y };
    // std::cout << mouse.getX() << " " << mouse.getY() << std::endl;
    //
    // const float t = (mouse - bottom).getDotProduct(axis) / axisLen2;
    // const float v = juce::jlimit(0.0f, 1.0f, t - mDragOffsetT);
    // s.attachment->setValueAsPartOfGesture(v);
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

