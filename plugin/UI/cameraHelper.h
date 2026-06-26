//
// Created by Erik Jourgensen on 6/26/26.
//
#pragma once
#include <cstring>
#include <cmath>

inline void buildLookAt(float out[16],
                        const float ex, float ey, float ez,
                        float tx, float ty, float tz)
    {
        float fx = tx-ex, fy = ty-ey, fz = tz-ez;
        const float fl = sqrtf(fx*fx + fy*fy + fz*fz);
        fx/=fl; fy/=fl; fz/=fl;

        // right = cross(worldUp=(0,1,0), forward)
        float rx = fz, ry = 0.0f, rz = -fx;
        const float rl = sqrtf(rx*rx + ry*ry + rz*rz);
        rx/=rl; ry/=rl; rz/=rl;

        // corrected up = cross(forward, right)
        const float ux = fy*rz - fz*ry;
        const float uy = fz*rx - fx*rz;
        float uz = fx*ry - fy*rx;

        // column-major: out[col*4+row]
        out[0]=rx;   out[4]=ux;   out[8] =-fx;  out[12]=-(rx*ex+ry*ey+rz*ez);
        out[1]=ry;   out[5]=uy;   out[9] =-fy;  out[13]=-(ux*ex+uy*ey+uz*ez);
        out[2]=rz;   out[6]=uz;   out[10]=-fz;  out[14]=  fx*ex+fy*ey+fz*ez;
        out[3]=0.0f; out[7]=0.0f; out[11]=0.0f; out[15]=1.0f;
    }

    // Right-handed perspective, Z maps to [0, 1] (WebGPU NDC)
inline void buildPerspective(float out[16], float fovY, float aspect, float zNear, float zFar)
    {
        const float f = 1.0f / tanf(fovY * 0.5f);
        std::memset(out, 0, 64);
        out[0]  = f / aspect;
        out[5]  = f;
        out[10] = zFar / (zNear - zFar);
        out[11] = -1.0f;
        out[14] = (zNear * zFar) / (zNear - zFar);
    }

inline void mulMat4(float out[16], const float a[16], const float b[16])
    {
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row) {
                float v = 0.0f;
                for (int k = 0; k < 4; ++k)
                    v += a[k*4+row] * b[col*4+k];
                out[col*4+row] = v;
            }
    }
