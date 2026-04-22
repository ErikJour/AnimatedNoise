//
// Created by Erik Jourgensen on 4/20/26.
//

//
// Created by Erik Jourgensen on 4/20/26.
//

#include <iostream>
#include <webgpu/webgpu.h>
#include <vector>
#include "utilityHelper.h"

//==============================================================================
//Variables
//==============================================================================
WGPUInstanceDescriptor descriptor = {};
WGPUInstance instance = nullptr;
WGPURequestAdapterOptions adapterOpts = {};
WGPUAdapterProperties initProperties = {};
std::vector<WGPUFeatureName> features;

//==============================================================================
//Adapter function
//==============================================================================
void setFeatures(const WGPUAdapter adapter)
{
    // Call the function a first time with a null return address, just to get
    // the entry count.
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

    // Allocate memory (could be a new, or a malloc() if this were a C program)
    features.resize(featureCount);

    // Call the function a second time, with a non-null return address
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec; // Restore decimal numbers
}

void getAdapter(const WGPUAdapter adapter, WGPUAdapterProperties &properties)
{
    std::cout << "Got adapter: " << adapter << std::endl;

    //Get name and properties
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter name: " << properties.name << std::endl;
    std::cout << "Adapter backend: " << properties.backendType << std::endl;
}

int main(int, char**)
{
    //=================================================================
    //Create webgpu instance - pluginEditor constructor
    //=================================================================
    descriptor.nextInChain = nullptr;
    instance = wgpuCreateInstance(&descriptor);

    //=================================================================
    //Check webgpu instance
    //=================================================================
    if (!instance)
    {
        std::cerr << "Failed to create WGPU instance." << std::endl;
        return 1;
    }

    std::cout << "Created WGPU instance." << instance << std::endl;

    //=================================================================
    //Get the adapter
    //=================================================================
    std::cout << "Requesting adapter..." << std::endl;
    adapterOpts.nextInChain = nullptr;
    const WGPUAdapter mAdapter = requestAdapterSync(instance, &adapterOpts);

    getAdapter(mAdapter, initProperties);

    //Get Limits
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

    bool success = wgpuAdapterGetLimits(mAdapter, &supportedLimits) == WGPUStatus_Success;
    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    }

    //=================================================================
    //Features
    //=================================================================
    setFeatures(mAdapter);

    //=================================================================
    //Properties
    //=================================================================
    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(mAdapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendorName) {
        std::cout << " - vendorName: " << properties.vendorName << std::endl;
    }
    if (properties.architecture) {
        std::cout << " - architecture: " << properties.architecture << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.name) {
        std::cout << " - name: " << properties.name << std::endl;
    }
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers


    //=================================================================
    //Destroy webgpu Adapter in editor's destructor
    //=================================================================
    std::cout << "Releasing adapter" << std::endl;
    wgpuAdapterRelease(mAdapter);
    //=================================================================
    //Destroy webgpu instance in editor's destructor
    //=================================================================
    std::cout << "Releasing instance" << std::endl;
    wgpuInstanceRelease(instance);


    return 0;
}

