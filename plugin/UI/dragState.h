//
// Created by Erik Jourgensen on 5/21/26.
//
#pragma once
#include "cameraState.h"

struct DragState {
    bool  active            = false;
    float startMouseX       = 0.0f;
    float startMouseY       = 0.0f;
    float startAngleX       = 0.0f;
    float startAngleY       = 0.0f;
    float sensitivity       = 0.01f;
    float scrollSensitivity = 0.1f;  // tune to taste
    vec2 velocity = {0.0, 0.0};
    vec2 previousDelta;
    float inertia = 0.9f;
};

