//
// Created by Erik Jourgensen on 6/4/26.
#ifndef ANIMATEDNOISE_ANIMATEDVACTROL_H
#define ANIMATEDNOISE_ANIMATEDVACTROL_H

#include <algorithm>
#include <cmath>

class AnimatedVactrol
{
    public:
        void prepare(const double sampleRate)
        {
            mSampleRate = static_cast<float>(sampleRate);
            updateCoefficients();
            reset();
        }

        void reset()
        {
            mCurrent = kMinCurrent;
        }

        void setAttackTime(const float seconds)  { mAttackTime  = std::max(seconds, 1e-4f); updateCoefficients(); }
        void setReleaseTime(const float seconds) { mReleaseTime = std::max(seconds, 1e-4f); updateCoefficients(); }

        void setControl(float norm)
        {
            norm = std::clamp(norm, 0.0f, 1.0f);
            mTargetCurrent = kMinCurrent + norm * (kMaxCurrent - kMinCurrent);
        }

        void strike(const float velocity)
        {
            const float norm = std::clamp(velocity, 0.0f, 1.0f);
            mCurrent         = kMinCurrent + norm * (kMaxCurrent - kMinCurrent);
            mTargetCurrent   = kMinCurrent;
        }

        void releaseGate()
        {
            mTargetCurrent = kMinCurrent;
        }

        float tick()
        {
            const bool rising = (mTargetCurrent > mCurrent);
            float coeff = rising ? mAttackCoeff : mReleaseCoeff;
            if (rising)
            {
                const float levelNorm = std::clamp(
                    (mCurrent - kMinCurrent) / (kMaxCurrent - kMinCurrent), 0.0f, 1.0f);
                coeff = coeff + (1.0f - coeff) * (0.5f * levelNorm);
            }

            mCurrent += coeff * (mTargetCurrent - mCurrent);
            mCurrent = std::clamp(mCurrent, kMinCurrent, kMaxCurrent);

            return currentToResistance(mCurrent);
        }

        float getRf() const { return currentToResistance(mCurrent); }

    private:
        static constexpr float kA = 3.464f;
        static constexpr float kB = 1136.212f;

        static constexpr float kMinCurrent = 10e-6f;
        static constexpr float kMaxCurrent = 40e-3f;

        float mSampleRate   = 44100.0f;
        float mAttackTime   = 0.012f;
        float mReleaseTime  = 0.250f;
        float mAttackCoeff  = 0.0f;
        float mReleaseCoeff = 0.0f;

        float mCurrent       = kMinCurrent;
        float mTargetCurrent = kMinCurrent;

        static float currentToResistance(const float If)
        {
            return kA / std::pow(If, 1.4f) + kB;   // Eq. 39
        }

        void updateCoefficients()
        {
            mAttackCoeff  = 1.0f - std::exp(-1.0f / (mAttackTime  * mSampleRate));
            mReleaseCoeff = 1.0f - std::exp(-1.0f / (mReleaseTime * mSampleRate));
        }
};

#endif //ANIMATEDNOISE_ANIMATEDVACTROL_H