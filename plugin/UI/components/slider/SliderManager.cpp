#include "SliderManager.h"
#include "sliderCatalog.h"
#include <iostream>
#include <cfloat>

void SliderManager::initializeSliders()
{
    const auto& defs = sliderDefinitions();
    mSliders.clear();
    mSliders.reserve(defs.size());

    for (const auto& [paramID, angle, materialId ] : defs)
    {
        auto& s        = mSliders.emplace_back();
        s.paramID      = paramID;
        s.angle        = angle;
        s.materialId   = materialId;

        auto* param = mApvts.getParameter(paramID.getParamID());
        jassert(param != nullptr);

        s.attachment = std::make_unique<juce::ParameterAttachment>(
            *param,
            [&s](const float newValue) { s.value = newValue; });

        s.attachment->sendInitialUpdate();
    }

    mScene.setSliderList(mSliders);
}

bool SliderManager::handleMouseDown(const juce::MouseEvent& event, const int width, const int height) const
{
    //Step 1 - Normalize coordinates -1 to 1
    const float x = (2.0f * static_cast<float>(event.x)) / static_cast<float>(width) - 1.0f;
    std::cout << "X is: " << x << std::endl;

    const float y = 1.0f - (2.0f * static_cast<float>(event.y)) / static_cast<float>(height);
    std::cout << "Y is: " << y << std::endl;

    //Step 2 - clip coordinates
    const float ray_clip[4] = { x, y, -1.0f, 1.0f };

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
    juce::ignoreUnused(rayOrigin);

    //Need slider positions somehow

    // TODO ray picking, per slider:
    //   1. intersect (rayOrigin, ray_wor) against slider's world-space bounds
    //   2. keep smallest positive t across sliders  -> bestIndex
    //   3. recover position along slider axis       -> bestTRaw
    //
    // then (unchanged from old picker):
    // if (bestIndex < 0) return false;
    // auto& s = mSliders[bestIndex];
    // mActiveSlider = bestIndex;
    // mDragOffsetT  = bestTRaw - s.value;
    // mDragging = true;  s.pressed = true;
    // const float v = juce::jlimit(0.0f, 1.0f, bestTRaw - mDragOffsetT);
    // s.attachment->beginGesture();
    // s.attachment->setValueAsPartOfGesture(v);

    return false;
}

bool SliderManager::handleMouseDrag(const juce::MouseEvent& /*event*/, int /*width*/, int /*height*/) const
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

