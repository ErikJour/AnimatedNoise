//
// Created by Erik Jourgensen on 6/4/26.
//
// Heuristic vactrol (Perkin Elmer VTL5C3/2) model.
// After Parker & D'Angelo, DAFx-13, Section 3.2 / Fig. 9.
//
// Models the two musically important behaviours of the photoresistive
// opto-isolator:
//   1. Asymmetric memory effect — resistance falls quickly when the LED
//      brightens (~12 ms) but rises slowly when it dims (~250 ms), and the
//      response speeds up at higher drive levels (datasheet behaviour).
//   2. Current-to-resistance mapping (Eq. 39): Rf = A / If^1.4 + B.
//
// Drive it with a control value in [0, 1] (e.g. velocity on note-on, decaying
// to 0). Output getRf() feeds directly into the LPG's setRf().
//

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
            // Start dark: tiny LED current => very high resistance (gate closed)
            mCurrent = kMinCurrent;
        }

        // Datasheet response times for the VTL5C3/2 (seconds).
        // Brightening (rising current) is fast; dimming (falling) is slow.
        void setAttackTime(const float seconds)  { mAttackTime  = std::max(seconds, 1e-4f); updateCoefficients(); }
        void setReleaseTime(const float seconds) { mReleaseTime = std::max(seconds, 1e-4f); updateCoefficients(); }

        // Control input in [0, 1]; maps to target LED current.
        // 0 -> dark/closed, 1 -> full drive/open.
        void setControl(float norm)
        {
            norm = std::clamp(norm, 0.0f, 1.0f);
            mTargetCurrent = kMinCurrent + norm * (kMaxCurrent - kMinCurrent);
        }

        // Trigger a strike: velocity in [0, 1] sets how hard the LED is driven.
        void strike(const float velocity)
        {
            setControl(std::clamp(velocity, 0.0f, 1.0f));
        }

        // Begin decay back toward darkness (e.g. on note-off, or always for a pluck).
        void releaseGate()
        {
            mTargetCurrent = kMinCurrent;
        }

        // Advance one sample. Returns the current vactrol resistance in ohms.
        float tick()
        {
            // Asymmetric one-pole: choose coefficient by direction of travel.
            const bool rising = (mTargetCurrent > mCurrent);
            float coeff = rising ? mAttackCoeff : mReleaseCoeff;

            // Datasheet note: the vactrol responds faster at higher light levels.
            // Nudge the effective coefficient toward faster as current rises.
            const float levelNorm = std::clamp(
                (mCurrent - kMinCurrent) / (kMaxCurrent - kMinCurrent), 0.0f, 1.0f);
            coeff = coeff + (1.0f - coeff) * (0.5f * levelNorm);

            mCurrent += coeff * (mTargetCurrent - mCurrent);
            mCurrent = std::clamp(mCurrent, kMinCurrent, kMaxCurrent);

            return currentToResistance(mCurrent);
        }

        float getRf() const { return currentToResistance(mCurrent); }

    private:
        // Eq. 39 component-fit constants.
        static constexpr float kA = 3.464f;       // Ohm * A^1.4
        static constexpr float kB = 1136.212f;    // Ohm

        // Paper current limits.
        static constexpr float kMinCurrent = 10e-6f;   // 10 uA  -> ~34 MOhm
        static constexpr float kMaxCurrent = 40e-3f;   // 40 mA  -> ~1.45 kOhm

        float mSampleRate   = 44100.0f;
        float mAttackTime   = 0.012f;   // 12 ms  (datasheet, brightening)
        float mReleaseTime  = 0.250f;   // 250 ms (datasheet, dimming)
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
            // One-pole time-constant -> per-sample coefficient.
            mAttackCoeff  = 1.0f - std::exp(-1.0f / (mAttackTime  * mSampleRate));
            mReleaseCoeff = 1.0f - std::exp(-1.0f / (mReleaseTime * mSampleRate));
        }
};

#endif //ANIMATEDNOISE_ANIMATEDVACTROL_H