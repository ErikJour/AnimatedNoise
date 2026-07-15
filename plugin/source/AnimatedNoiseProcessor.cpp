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
    castParameter(apvts, ParameterID::noiseLevel, noiseLevelParam);
    castParameter(apvts, ParameterID::noiseDensity, noiseDensityParam);
    castParameter(apvts, ParameterID::lpgResonance, lpgResonanceParam);
    castParameter(apvts, ParameterID::lpgVactrolRelease, lpgVactrolReleaseParam);
    castParameter(apvts, ParameterID::envAttack, envelopeAttackParam);
    castParameter(apvts, ParameterID::envDecay, envelopeDecayParam);
    castParameter(apvts, ParameterID::envSustain, envelopeSustainParam);
    castParameter(apvts, ParameterID::envRelease, envelopeReleaseParam);
    castParameter(apvts, ParameterID::envRelease, gainParam);

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
    return 1;

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
    parametersChanged.store(true);

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
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

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
            const uint8_t data1 = (metadata.numBytes >= 2) ? metadata.data[1] : 0;
            const uint8_t data2 = (metadata.numBytes == 3) ? metadata.data[2] : 0;
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
    //==========================================================
    //Global Params
    //==========================================================
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::gain,
        "Master Gain",
        juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f, 1.0f },
        0.5f));

    //==========================================================
    //Noise Level
    //==========================================================
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::noiseLevel,
        "Noise Level",
        juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f, 1.0f },
        0.5f));
    //==========================================================
    //Noise Density
    //==========================================================
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::noiseDensity,
        "Noise Density",
        juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f, 1.0f },
        0.5f));
    //==========================================================
    //Lpg
    //==========================================================
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::lpgResonance,
        "LPG Resonance",
        juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f, 1.0f },
        0.5f));
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::lpgVactrolRelease,
        "LPG Vactrol Release",
        juce::NormalisableRange<float>{ 0.05f, 1.0f, 0.01f, 1.0f },
        0.5f));
    //==========================================================
    //Amp Envelope
    //==========================================================
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::envAttack,
        "Env Attack",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f, 1.5f),
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::envDecay,
        "Env Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::envSustain,
        "Env Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::envRelease,
        "Env Release",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return paramLayout;
}

void AnimatedNoiseProcessor::update()
{
    //=======================================
    //Global Gain
    //=======================================
    const float gain = gainParam->get();
    noiseSynth.setGain(gain);
    std::cout << "gain in processor : " << gain << std::endl;
    //=======================================
    //Noise Level
    //=======================================
    const float noiseLevel = noiseLevelParam->get();
    noiseSynth.setNoiseLevel(noiseLevel);
    //=======================================
    //Noise Density
    //=======================================
    const float noiseDensity = noiseDensityParam->get();
    noiseSynth.setNoiseDensity(noiseDensity);
    //=======================================
    //LPG Resonance
    //=======================================
    const float lpgResonance = lpgResonanceParam->get();
    noiseSynth.setLpgResonance(lpgResonance);
    const auto sampleRate = static_cast<float>(getSampleRate());
    const float inverseSampleRate = 1.0f / sampleRate;
    const float lpgVactrolRelease = lpgVactrolReleaseParam->get();
    noiseSynth.setLpgVactrolRelease(lpgVactrolRelease);
    //=======================================
    //Amp Envelope
    //=======================================
    noiseSynth.envAttack = std::exp(-inverseSampleRate * std::exp(4.5f - 0.075f * envelopeAttackParam->get()));
    noiseSynth.envDecay = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envelopeDecayParam->get()));
    const float ampEnvSustain = envelopeSustainParam->get() / 100.0f;
    noiseSynth.setSustain(ampEnvSustain);

    if (const float envRelease = envelopeReleaseParam->get(); envRelease < 1.0f) { noiseSynth.envRelease = 0.75f; }
    else { noiseSynth.envRelease = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envRelease)); }
    const float release = noiseSynth.envRelease;
    static float prevR=-1;
    if (std::fabsf(release - prevR) > 1e-6f) {
        noiseSynth.setRelease(release);
        prevR = release;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnimatedNoiseProcessor();
}
