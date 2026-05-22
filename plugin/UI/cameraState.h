//
// Created by Erik Jourgensen on 5/14/26.
//

#ifndef ANIMATEDNOISE_CAMERASTATE_H
#define ANIMATEDNOISE_CAMERASTATE_H

struct vec2 { float x,y; };

struct CameraState {
    float angleX = 0.0f;   // azimuth
    float angleY = 0.1f;   // elevation
    float zoom   = -0.5f;
};

#endif //ANIMATEDNOISE_CAMERASTATE_H