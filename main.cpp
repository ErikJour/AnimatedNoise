//
// Created by Erik Jourgensen on 4/20/26.
//

//
// Created by Erik Jourgensen on 4/20/26.
//

#ifndef ANIMATEDNOISE_MAIN_H
#include <iostream>
#include <webgpu/webgpu.h>
#include <cassert>
#include <vector>


#define ANIMATEDNOISE_MAIN_H

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    // We wait until userData.requestEnded gets true
    // {{Wait for request to end}}

    assert(userData.requestEnded);

    return userData.adapter;
}


int main(int, char**)
{
    //=================================================================
    //Create webgpu instance - pluginEditor constructor
    //=================================================================
    WGPUInstanceDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;

    const WGPUInstance instance = wgpuCreateInstance(&descriptor);

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

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    const WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

    std::cout << "Got adapter: " << adapter << std::endl;

    //Get name and properties
    WGPUAdapterProperties initProperties = {};
    wgpuAdapterGetProperties(adapter, &initProperties);
    std::cout << "Adapter name: " << initProperties.name << std::endl;
    std::cout << "Adapter backend: " << initProperties.backendType << std::endl;

    //Get Limits
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
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
    std::vector<WGPUFeatureName> features;

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

    //=================================================================
    //Properties
    //=================================================================
    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
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
    wgpuAdapterRelease(adapter);
    //=================================================================
    //Destroy webgpu instance in editor's destructor
    //=================================================================
    std::cout << "Releasing instance" << std::endl;
    wgpuInstanceRelease(instance);


    return 0;
}

#endif //ANIMATEDNOISE_MAIN_H