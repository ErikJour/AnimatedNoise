#include "AnimatedNoiseProcessor.h"
#include "AnimatedNoiseEditor.h"

//==============================================================================
AnimatedNoiseProcessor::AnimatedNoiseProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    apvts.state.addListener(this);
    castParameter(apvts, ParameterID::globalGain, globalGainParam);
}

AnimatedNoiseProcessor::~AnimatedNoiseProcessor()
{
    apvts.state.removeListener(this);

}

//==============================================================================
const juce::String AnimatedNoiseProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnimatedNoiseProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnimatedNoiseProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnimatedNoiseProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnimatedNoiseProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnimatedNoiseProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AnimatedNoiseProcessor::getCurrentProgram()
{
    return 0;
}

void AnimatedNoiseProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AnimatedNoiseProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AnimatedNoiseProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AnimatedNoiseProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    noiseSynth.distributeResources(sampleRate, samplesPerBlock);
    noiseSynth.reset(sampleRate);
}

void AnimatedNoiseProcessor::releaseResources()
{
}

bool AnimatedNoiseProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AnimatedNoiseProcessor::handleMidi(const uint8_t data0, const uint8_t data1, const uint8_t data2)
{
    noiseSynth.midiMessages(data0, data1, data2);
}

void AnimatedNoiseProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; i++)
        buffer.clear(i, 0, buffer.getNumSamples());

    bool expected = true;

    if (parametersChanged.compare_exchange_strong(expected, false))
        update();

    splitBufferByEvents(buffer, midiMessages);

}

void AnimatedNoiseProcessor::render(juce::AudioBuffer<float>& buffer, int sampleCount, int bufferOffset)
{
    float* outputBuffers[2] = { nullptr, nullptr };

    outputBuffers[0] = buffer.getWritePointer(0) + bufferOffset;

    if (getTotalNumOutputChannels() > 1) {
        outputBuffers[1] = buffer.getWritePointer(1) + bufferOffset;
    }

    noiseSynth.render(outputBuffers, sampleCount);
}

void AnimatedNoiseProcessor::splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    int bufferOffset = 0;

    for (const auto metadata : midiMessages) {

        int samplesThisSegment = metadata.samplePosition - bufferOffset;

        if (samplesThisSegment > 0) {

            render(buffer, samplesThisSegment, bufferOffset);

            bufferOffset += samplesThisSegment;
        }
        if (metadata.numBytes <= 3) {
            uint8_t data1 = (metadata.numBytes >= 2) ? metadata.data[1] : 0;
            uint8_t data2 = (metadata.numBytes == 3) ? metadata.data[2] : 0;
            handleMidi(metadata.data[0], data1, data2);
        }
    }

    int samplesLastSegment = buffer.getNumSamples() - bufferOffset;
    if (samplesLastSegment > 0) {
        render(buffer, samplesLastSegment, bufferOffset);
    }

    midiMessages.clear();
}

//==============================================================================
bool AnimatedNoiseProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AnimatedNoiseProcessor::createEditor()
{
    return new AnimatedNoiseProcessorEditor (*this);
}

//==============================================================================
void AnimatedNoiseProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void AnimatedNoiseProcessor::setStateInformation (const void* data, int sizeInBytes)
{

    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout AnimatedNoiseProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout paramLayout;
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::globalGain,
        "Global Gain",
        juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f, 1.0f },
        0.5f));

    return paramLayout;
}

void AnimatedNoiseProcessor::update()
{
    //=======================================
    //Global Gain
    //=======================================
    const float globalGain = globalGainParam->get();
    noiseSynth.setGain(globalGain);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnimatedNoiseProcessor();
}
