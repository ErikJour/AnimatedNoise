#pragma once
#include <cstddef>
#include <cstdint>

struct MyUniforms {
    float         time;
    float         frequency;
    float         amplitude;
    float         sliderValue;
    float         lightPos[3];
    float         _pad1;
    float         sliderPos[3];
    float         _pad2;
};

static_assert(sizeof(MyUniforms) % 16 == 0);

