//
// Created by Erik Jourgensen on 7/21/26.
//

#pragma once
#include <cstdint>

constexpr float SILENT = 0.0001f;

enum class Stage { Delay, Idle, Attack, Release };

class AnimatedAuxEnv
{
public:

    AnimatedAuxEnv();
    void reset();
    void attack();
    void release();
    float nextValue();
    void quickRelease(double sampleRate);
    void adoptReleaseNow(float m);
    void setReleaseMultiplier(float newRelease);
    void setAttackMultiplier(float newAttack);
    void setDrift(float newDrift);
    [[nodiscard]] inline bool isActive() const;
    [[nodiscard]] inline bool isInAttack() const;
    //======================================
    //Get Value for UI
    //======================================
    [[nodiscard]] float getCurrentArValue() const;
    //======================================
    //Public Members
    //======================================
    float level;
    float attackMultiplier;
    float releaseMultiplier;
    bool loop {false};
    float drift;
    int delay;
private:
    //======================================
    //Private Members
    //======================================
    float multiplier;
    float target;
    Stage stage;
    //======================================
    //Drift
    //======================================
    uint32_t randomState {123456789u};
    float bipolarRandom();
    float applyDrift(float baseMult);
    //==============================================================================
};







