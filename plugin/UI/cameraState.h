//
// Created by Erik Jourgensen on 5/14/26.
//

#ifndef ANIMATEDNOISE_CAMERASTATE_H
#define ANIMATEDNOISE_CAMERASTATE_H

struct vec2 { float x,y; };

struct CameraState {
    // angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
    // angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
    vec2 angles = { 0.8f, 0.5f };
    // zoom is the position of the camera along its local forward axis, affected by the scroll wheel
    float zoom = -1.2f;
};

#endif //ANIMATEDNOISE_CAMERASTATE_H