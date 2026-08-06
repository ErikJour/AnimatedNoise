//
// Created by Erik Jourgensen on 8/6/26.
//

#pragma once


class AnimatedFilter
{
public:
    AnimatedFilter();
    ~AnimatedFilter();
    //======================
    //Public member functions
    //======================
    void reset();
    static float onePoleIIR(float input, float delay, float coeffA, float coeffB);
    void processBuffer(float* buffer, int numSamples);
    //======================
    //Public variables
    //======================
    float mCoeffA = 0.0f;
    float mCoeffB = 0.0f;
private:
    float mDelay  = 0.0f;

};


