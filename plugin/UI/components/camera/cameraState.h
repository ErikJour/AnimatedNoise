//
// Created by Erik Jourgensen on 5/14/26.
//

#ifndef ANIMATEDNOISE_CAMERASTATE_H
#define ANIMATEDNOISE_CAMERASTATE_H

struct vec2 { float x, y; };

struct CameraState {
    float angleX = 0.0f;   // horizontal look direction (yaw)
    float posX   = 0.0f;   // camera world position X
    float posZ   = 1.0f;   // camera world position Z

    static constexpr float eyeY        = 0.02f;  // fixed eye height
    static constexpr float kWallRadius = 2.0f;  // clamp radius (floor edge is 0.95)
};

#endif //ANIMATEDNOISE_CAMERASTATE_H