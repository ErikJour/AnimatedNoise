#ifndef ANIMATEDNOISE_SLIDERMANAGER_H
#define ANIMATEDNOISE_SLIDERMANAGER_H
#include "../../Scene.h"
#include "AnimatedSlider.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class SliderManager
{
public:
    explicit SliderManager(Scene& scene, juce::AudioProcessorValueTreeState& apvts)
        : mScene(scene), mApvts(apvts) {}
    ~SliderManager() = default;

    void initializeSliders();
    bool handleMouseDown(const juce::MouseEvent& event, int width, int height);
    bool handleMouseDrag(const juce::MouseEvent& event, int width, int height) const;
    bool handleMouseUp();

    const std::vector<AnimatedSlider>& sliders() const { return mSliders; }

private:
    // Column-major mat4 * vec4  (matches your matrix layout)
    static inline void mulMat4Vec4(float* out, const float* m, const float* v)
    {
        for (int row = 0; row < 4; ++row)
            out[row] = m[0*4 + row] * v[0]
                     + m[1*4 + row] * v[1]
                     + m[2*4 + row] * v[2]
                     + m[3*4 + row] * v[3];
    }

    Scene&                                mScene;
    std::vector<AnimatedSlider>           mSliders;
    juce::AudioProcessorValueTreeState&   mApvts;
    int                                   mActiveSlider = -1;
    float mDragOffsetT = 0.0f;
    bool                                  mDragging     = false;
};

#endif