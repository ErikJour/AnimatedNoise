//
// Created by Erik Jourgensen on 5/20/26.
//

#include "FunctionGenerator.h"

FunctionGenerator::FunctionGenerator() : mLevel(0.0f), mAttackMultiplier(0.99f), mReleaseMultiplier(0.99f),
                                        mLoop(false), mDrift(0.0f), mDelay(0.0f), mMultiplier(0.0f), mTarget(0),
                                         mStage(Stage::Idle), mDriftPhase(0.0f), mDriftRate(0.0f)
{
}

FunctionGenerator::~FunctionGenerator() {}

void FunctionGenerator::reset()
{
    mLevel = 0.0f;
    mTarget = 0.0f;
    mMultiplier = 0.0f;
    mStage = Stage::Idle;
}
void FunctionGenerator::attack()
{
    mStage = Stage::Attack;
    mTarget = 2.0f;
    mMultiplier = applyDrift(mAttackMultiplier);
}
void FunctionGenerator::release()
{
    mStage = Stage::Release;
    mTarget = 0.0f;
    mMultiplier = mReleaseMultiplier;
;
}
float FunctionGenerator::nextValue()
{
    mLevel = analogCurve(mLevel, mTarget, mMultiplier);

    if (mStage == Stage::Attack && mLevel >= 1.0f) release();

    if (mStage == Stage::Release && mLevel <= SILENT)
    {
        if (mLoop)
            attack();
        else
            mStage = Stage::Idle;
    }

    if (mStage == Stage::Idle && mLoop && mLevel <= SILENT) { attack(); }

    return mLevel;
}

void FunctionGenerator::setAttackMultiplier(const float newAttack)
{
    mAttackMultiplier = newAttack;

    if (mStage == Stage::Attack)
        mMultiplier = mAttackMultiplier;
}

void FunctionGenerator::setReleaseMultiplier(const float newRelease)
{
    mReleaseMultiplier = newRelease;

    if (mStage == Stage::Release)
        mMultiplier = mReleaseMultiplier;
}

float FunctionGenerator::analogCurve(const float level, const float target, const float multiplier)
{
    const float diff = target - level;
    const float linearCurrent = std::tanh(diff / kOTAKnee);
    return level + linearCurrent * (1.0f - multiplier);
}

void FunctionGenerator::process(float* buffer, const int numSamples)
{
    for (int i = 0; i < numSamples; i++)
        buffer[i] *= nextValue();
}


