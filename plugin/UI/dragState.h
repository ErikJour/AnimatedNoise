//
// Created by Erik Jourgensen on 5/21/26.
//
#pragma once
#include "cameraState.h"

struct DragState {
    bool  active            = false;
    float startMouseX       = 0.0f;
    float startAngleX       = 0.0f;
    float sensitivity       = 0.005f;
    float scrollSensitivity = 0.05f;
    float turnSensitivity   = 2.0f;
};

