#pragma once

constexpr float SILENCE = 0.0001f;

enum class EnvStage { Idle, Attack, Decay, Sustain, Release };

class Envelope
{
public:
    Envelope() = default;

    void reset()
    {
        level = 0.0f;
        target = 0.0f;
        multiplier = 0.0f;
        stage = EnvStage::Idle;
    }

    void attack()
    {
        level += SILENCE + SILENCE;
        target = 2.0f;
        multiplier = attackMultiplier;
        stage = EnvStage::Attack;
    }

    void release()
    {
        target = 0.0f;
        multiplier = releaseMultiplier;
        stage = EnvStage::Release;
    }

    float nextValue()
    {
        level = multiplier * (level - target) + target;

        if (stage == EnvStage::Attack && level + target > 3.0f)
        {
            stage = EnvStage::Decay;
            multiplier = decayMultiplier;
            target = sustainLevel;
        }

        return level;
    }

    void quickRelease(const double sampleRate)
    {
        target = 0.0f;
        constexpr float quickTime = 0.005f;
        multiplier = std::expf(-1.0f / ((static_cast<float>(sampleRate) * quickTime)));
        stage = EnvStage::Release;
    }

    void setReleaseMultiplier(const float newRelease)
    {
        releaseMultiplier = newRelease;  // Store actual value for future releases

        if (stage == EnvStage::Release)
        {
            // Already-releasing voices: respond to changes but don't die instantly
            constexpr float minLiveMult = 0.9995f;  // ~50ms minimum at 48kHz
            multiplier = std::max(releaseMultiplier, minLiveMult);
        }
    }

    void setSustain(const float newSustain)
    {
        sustainLevel = newSustain;

        if (stage == EnvStage::Decay || stage == EnvStage::Sustain)
        {
            target = sustainLevel;
        }
    }

    void process(float* buffer, const int numSamples)
    {
        for (int i = 0; i < numSamples; i++)
            buffer[i] *= nextValue();
    }

    [[nodiscard]] inline bool isActive() const { return level > SILENCE; }
    [[nodiscard]] inline bool isInAttack() const { return stage == EnvStage::Attack; }
    [[nodiscard]] float currentValue() const { return level; }

    float level{};
    float attackMultiplier{};
    float decayMultiplier{};
    float sustainLevel{};
    float releaseMultiplier{};

private:
    float multiplier{};
    float target{};
    EnvStage stage { EnvStage::Idle };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Envelope)

};