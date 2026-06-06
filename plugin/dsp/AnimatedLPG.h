//
// Created by Erik Jourgensen on 6/4/26.
//

#ifndef ANIMATEDNOISE_ANIMATEDLPG_H
#define ANIMATEDNOISE_ANIMATEDLPG_H
#include <algorithm>

class AnimatedLPG
{
    public:
        enum class Mode { Both, VCA, LowPass};

        void prepare(const double sampleRate)
        {
            mSampleRate = static_cast<float>(sampleRate);
            Rf = 1e3f;
            resonanceSmoothed.reset(mSampleRate, 0.01f);
            resonanceSmoothed.setCurrentAndTargetValue(0.5f);
            reset();
        }

        void reset() { sVx = sVout = sDiff = 0.0f; }

        void setMode(const Mode mode)
        {
            mMode = mode;
            switch (mode)
            {
            case Mode::Both:     C1 = 1e-9f;    C2 = 1e-9f;    C3 = 0.0f;    Ra = 5e6f; break;
            case Mode::VCA:      C1 = 220e-12f; C2 = 220e-12f; C3 = 0.0f;    Ra = 5e3f; break;
            case Mode::LowPass:  C1 = 1e-9f;    C2 = 220e-12f; C3 = 4.7e-9f; Ra = 5e6f; break;
            default: ;
            }
        }
        void setRf(const float resistance) { Rf = resistance; }

        void setResonance(const float newResonance)
        {
            resonanceSmoothed.setTargetValue(newResonance);
            float resonance = resonanceSmoothed.getCurrentValue();
            resonance = std::clamp(resonance, 0.0f, 0.999f);
            a = (C3 > 0.0f) ? resonance * getAmax() : 0.0f;
        }

        [[nodiscard]] float getAmax() const
        {
            if (C3 <= 0.0f) return 0.0f;
            return (2.0f * C1 * Ra + (C2 + C3) * (Ra + Rf)) / (C3 * Ra);
        }

        [[nodiscard]] float getRf() const { return Rf; }

        void processBuffer(float* buffer, const int numSamples)
        {
            const float a1 = 1.0f / (C1 * Rf);
            const float a2 = -(1.0f / C1) * (1.0f / Rf + 1.0f / Ra);
            const float b1 = 1.0f  / (C2 * Rf);
            const float b2 = -2.0f / (C2 * Rf);
            const float b3 = 1.0f  / (C2 * Rf);
            const float b4 = C3 / C2;
            const float d1 = a;
            constexpr float d2 = -1.0f;

            const float h = 1.0f / (2.0f * static_cast<float>(mSampleRate));
            const float g = 2.0f * static_cast<float>(mSampleRate);

            const float D = 1.0f - h * a2;
            const float P = 1.0f - h * b2 - h * b4 * g * d2;
            const float Q = h * b3 + h * b4 * g * d1;
            const float denomYx = P - Q * h * a1 / D;

            for (int i = 0; i < numSamples; i++)
            {
                buffer[i] = processSample(buffer[i], a1, a2, b1, b2, b3, b4, d1, d2, h, g, D, Q, denomYx);
            }

        }

    template <typename RfSource>
    void processBufferModulated(float* buffer, const int numSamples, RfSource&& nextRf)
            {
                const float h = 1.0f / (2.0f * static_cast<float>(mSampleRate));
                const float g = 2.0f * static_cast<float>(mSampleRate);

                for (int i = 0; i < numSamples; ++i)
                {
                    Rf = nextRf();

                    const float a1 = 1.0f / (C1 * Rf);
                    const float a2 = -(1.0f / C1) * (1.0f / Rf + 1.0f / Ra);
                    const float b1 = 1.0f / (C2 * Rf);
                    const float b2 = -2.0f / (C2 * Rf);
                    const float b3 = 1.0f / (C2 * Rf);
                    const float b4 = C3 / C2;
                    const float d1 = a;
                    constexpr float d2 = -1.0f;

                    const float D = 1.0f - h * a2;
                    const float P = 1.0f - h * b2 - h * b4 * g * d2;
                    const float Q = h * b3 + h * b4 * g * d1;
                    const float denomYx = P - Q * h * a1 / D;

                    buffer[i] = processSample(buffer[i], a1, a2, b1, b2, b3, b4, d1, d2, h, g, D, Q, denomYx);
                }
            }

        float processSample(const float yi,
                            const float a1,
                            const float a2,
                            const float b1,
                            const float b2,
                            const float b3,
                            const float b4,
                            const float d1,
                            const float d2,
                            const float h,
                            const float g,
                            const float D,
                            const float Q,
                            const float denomYx)
        {
            const float yx = (sVx + h * b1 * yi + Q * sVout / D + h * b4 * sDiff) / denomYx;
            const float yo = (sVout + h * a1 * yx) / D;

            const float ud = d1 * yo + d2 * yx;
            const float yd = g * ud + sDiff;

            const float ux = b1 * yi + b2 * yx + b3 * yo + b4 * yd;
            const float uo = a1 * yx + a2 * yo;

            sVx = yx + h * ux;
            sVout = yo + h * uo;
            sDiff = -yd - g * ud;

            return yo;
        }
    private:

        float C1 = 1e-9f;
        float C2 = 220e-12f;
        float C3 = 4.7e-9f;
        float Ra = 5e6f;
        float Rf = 100e3f;
        float a = 0.0f;

        double mSampleRate = { 44100.0 };
        float sVx = 0.0f;
        float sVout = 0.0f;
        float sDiff = 0.0f;
        Mode mMode = Mode::LowPass;

        juce::SmoothedValue<float> resonanceSmoothed;

};

#endif //ANIMATEDNOISE_ANIMATEDLPG_H