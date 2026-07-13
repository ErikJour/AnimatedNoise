//
// Created by Erik Jourgensen on 5/21/26.
//
#pragma once
#include "../camera/cameraState.h"

struct DragState {
    bool  active            = false;
    float startMouseX       = 0.0f;
    float startAngleX       = 0.0f;
    float sensitivity       = 0.005f;
    float scrollSensitivity = 0.075f;
    float turnSensitivity   = 5.0f;
};

