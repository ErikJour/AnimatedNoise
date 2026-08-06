//
// Created by Erik Jourgensen on 7/21/26.
//

#include "AnimatedAuxEnv.h"
#include <algorithm>
#include <cmath>

AnimatedAuxEnv::AnimatedAuxEnv() : level(0.0f),
                                   attackMultiplier(0.999f),
                                   releaseMultiplier(0.999f),
                                   drift(0.0f),
                                   delay(0.0f),
                                   multiplier(0.0f),
                                   target(0.0f)
    {
        stage = Stage::Idle;

    }

    void AnimatedAuxEnv::reset()
    {
        level      = 0.0f;
        target     = 0.0f;
        multiplier = 0.0f;
        stage      = Stage::Idle;
    }

    void AnimatedAuxEnv::attack()
    {
        stage      = Stage::Attack;
        target     = 2.0f;
        multiplier = applyDrift(attackMultiplier);
    }

    void AnimatedAuxEnv::release()
    {
        stage      = Stage::Release;
        target     = 0.0f;
        multiplier = applyDrift(releaseMultiplier);
    }

    float AnimatedAuxEnv::nextValue ()
    {
        level = multiplier * (level - target) + target;

        if (stage == Stage::Attack && level >= 1.0f)
            release();

        if (stage == Stage::Release && level <= SILENT)
        {
            if (loop)
                attack();
            else
                stage = Stage::Idle;
        }

        if (stage == Stage::Idle && loop && level <= SILENT)
        {
            attack();
        }

        return level;
    }

    void AnimatedAuxEnv::quickRelease(const double sampleRate)
    {
        target                    = 0.0f;
        constexpr float quickTime = 0.005f;
        multiplier                = std::expf(-1.0f / ((static_cast<float>(sampleRate) * quickTime)));
    }

    void AnimatedAuxEnv::adoptReleaseNow(const float m)
    {
        constexpr float maxMult = 0.9995f;
        releaseMultiplier       = m;
        if (target == 0.0f)
            multiplier = std::min(applyDrift(releaseMultiplier), maxMult);
    }

    //======================================
    //Setters
    //======================================
    void AnimatedAuxEnv::setReleaseMultiplier(const float newRelease)
    {
        releaseMultiplier = newRelease;

        if (stage == Stage::Release)
            multiplier = applyDrift(releaseMultiplier);
    }

    void AnimatedAuxEnv::setAttackMultiplier(const float newAttack)
    {
        attackMultiplier = newAttack;

        if (stage == Stage::Attack)
            multiplier = applyDrift(attackMultiplier);

    }

    void AnimatedAuxEnv::setDrift(const float newDrift) { drift = newDrift; }


    [[nodiscard]] inline bool AnimatedAuxEnv::isActive() const   { return level > SILENT; }

    [[nodiscard]] inline bool AnimatedAuxEnv::isInAttack() const { return stage == Stage::Attack; }
    //======================================
    //Get Value for UI
    //======================================
    [[nodiscard]] float AnimatedAuxEnv::getCurrentArValue() const { return level; }
    //======================================
    //Public Members
    //======================================


    float AnimatedAuxEnv::bipolarRandom()
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return static_cast<float>(randomState) / static_cast<float>(UINT32_MAX) * 2.0f - 1.0f;
    }

    float AnimatedAuxEnv::applyDrift(const float baseMult)
    {
        if (drift <= 0.0f)
            return baseMult;
        constexpr float maxDrift = 0.15f;
        const float driftRange = drift * maxDrift;
        const float randomNumber = bipolarRandom();
        const float logBase = std::log(baseMult);
        const float drifted = logBase * (1.0f + randomNumber * driftRange);
        return std::exp(drifted);
    }








