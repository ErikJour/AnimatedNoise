#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <webgpu/webgpu.h>
#include "PluginProcessor.h"
#include "utilityHelper.h"

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    //WebGPU Functions
    //==============================================================================
    static void setFeatures(const WGPUAdapter adapter)
    {
        // Call the function a first time with a null return address, just to get
        // the entry count.
        WGPUSupportedFeatures supported = {};
        wgpuAdapterGetFeatures(adapter, &supported);

        // Call the function a second time, with a non-null return address
        std::cout << "Adapter features:" << std::endl;
        std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
        for (size_t i = 0; i < supported.featureCount; ++i) {
            std::cout << " - 0x" << supported.features[i] << std::endl;
        }
        std::cout << std::dec; // Restore decimal numbers
    }

    void getAdapter(const WGPUAdapter adapter, const WGPUAdapterInfo &properties)
    {
        std::cout << "Got adapter: " << adapter << std::endl;
        wgpuAdapterGetInfo(adapter, &initProperties);
        std::cout << "Adapter name: ";
        std::cout.write(properties.device.data, static_cast<std::streamsize>(properties.device.length));
        std::cout << std::endl;
        std::cout << "Adapter backend: " << properties.backendType << std::endl;
    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    //==============================================================================
    //WebGPU Variables
    //==============================================================================
    WGPUInstanceDescriptor descriptor = {};
    WGPUInstance mInstance = nullptr;
    WGPURequestAdapterOptions adapterOpts = {};
    WGPUAdapterInfo initProperties = {};
    WGPUDevice mDevice = nullptr;
    WGPUQueue mQueue = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
