#pragma once
#include <vector>

//===============================================
//Position and Color data
//===============================================


inline std::vector<float> positionData = {
    -0.5f, -0.5f,
    +0.5f, -0.5f,
     0.0f, +0.5f,
    -0.55f, -0.5f,
    -0.05f, +0.5f,
    -0.55f, +0.5f
};

inline std::vector<float> colorData = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f
};

// Define point data
// The de-duplicated list of point positions
// inline std::vector<float> pointData = {
//     // x,   y,     r,   g,   b
//     -0.5, -0.5,   1.0, 0.0, 0.0,
//     +0.5, -0.5,   0.0, 1.0, 0.0,
//     +0.5, +0.5,   0.0, 0.0, 1.0,
//     -0.5, +0.5,   1.0, 1.0, 0.0
// };
//
// // Define index data
// // This is a list of indices referencing positions in the pointData
// inline std::vector<uint16_t> indexData = {
//     0, 1, 2, // Triangle #0 connects points #0, #1 and #2
//     0, 2, 3  // Triangle #1 connects points #0, #2 and #3
// };