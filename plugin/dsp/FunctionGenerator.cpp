//
// Created by Erik Jourgensen on 5/20/26.
//

#include "FunctionGenerator.h"

FunctionGenerator::FunctionGenerator() : mLevel(0), mAttackMultiplier(0), mReleaseMultiplier(0), mDrift(0), mDelay(0),
                                         mMultiplier(0), mTarget(0),
                                         mStage(), mDriftPhase(0), mDriftRate(0)
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
    mMultiplier = applyDrift(mReleaseMultiplier);
;
}
float FunctionGenerator::nextValue()
{
    // mLevel = mMultiplier * (mLevel - mTarget) + mTarget;
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


