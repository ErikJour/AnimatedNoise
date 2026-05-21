//
// Created by Erik Jourgensen on 5/20/26.
//

#ifndef ANIMATEDNOISE_FUNCTIONGENERATOR_H
#define ANIMATEDNOISE_FUNCTIONGENERATOR_H
#include <cstdint>
#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>


constexpr float SILENT = 0.0001f;
static constexpr float kOTAKnee = 0.052f / 10.0f;  // ~0.005f
enum class Stage { Delay, Idle, Attack, Release };


class FunctionGenerator
{
    public:
        FunctionGenerator();
        ~FunctionGenerator();
        void reset();
        void setAttackMultiplier(float newAttack);
        void setReleaseMultiplier(float newRelease);
        void attack();
        void release();
        float nextValue();
        static float analogCurve(float level, float target, float multiplier);

        void setDrift(const float newDrift) { mDrift = newDrift; }
        [[nodiscard]] inline bool isActive() const { return mLevel > SILENT; }
        [[nodiscard]] inline bool isInAttack() const { return mStage == Stage::Attack; }
        [[nodiscard]] float getCurrentArValue() const { return mLevel; }

        float    mLevel;
        float    mAttackMultiplier;
        float    mReleaseMultiplier;
        bool     mLoop { false };
        float    mDrift;
        int      mDelay;

    private:
        float    mMultiplier;
        float    mTarget;
        Stage    mStage;
        uint32_t mRandomState {123456789u};
        float    mDriftPhase;
    float       mDriftRate;

        float bipolarRandom()
        {
            mRandomState ^= mRandomState << 13;
            mRandomState ^= mRandomState >> 17;
            mRandomState ^= mRandomState << 5;
            return static_cast<float>(mRandomState) / static_cast<float>(UINT32_MAX) * 2.0f - 1.0f;
        }

        float applyDrift(float multiplier)
        {
                mDriftPhase = std::fmod(mDriftPhase + mDriftRate, juce::MathConstants<float>::twoPi);
                return multiplier * (1.0f + mDrift * std::sin(mDriftPhase));
        }

    };


#endif //ANIMATEDNOISE_FUNCTIONGENERATOR_H