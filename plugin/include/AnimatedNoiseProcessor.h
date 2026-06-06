#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "NoiseSynth.h"
#include "cameraState.h"
#include "ParamIds.h"

//==============================================================================

class AnimatedNoiseProcessor final : public juce::AudioProcessor,
                                        private juce::ValueTree::Listener
{
public:
    //==============================================================================
    AnimatedNoiseProcessor();
    ~AnimatedNoiseProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    //    using AudioProcessor::processBlock;

    void splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void handleMidi(uint8_t data0, uint8_t data1, uint8_t data2);
    void render(juce::AudioBuffer<float>& buffer, int sampleCount, int bufferOffset);
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    NoiseSynth& getNoiseSynth() { return noiseSynth; }

    CameraState savedCameraState;

    juce::AudioProcessorValueTreeState apvts {*this,
                                nullptr,
                                "Parameters",
                                                createParameterLayout() };

private:
    NoiseSynth noiseSynth;
    juce::AudioParameterFloat* noiseLevelParam{};
    juce::AudioParameterFloat* combLevelParam{};
    juce::AudioParameterFloat* lpgResonanceParam{};

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override { parametersChanged.store(true); }
    void update();
    std::atomic<bool> parametersChanged { false };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedNoiseProcessor)
};
