//
// Created by Erik Jourgensen on 6/3/26.
//

#ifndef ANIMATEDNOISE_ANIMATEDSLIDER_H
#define ANIMATEDNOISE_ANIMATEDSLIDER_H

struct AnimatedSlider
{
    float angle{};
    const char* paramID{};
    float value = 0.0f;
};

#endif //ANIMATEDNOISE_ANIMATEDSLIDER_H