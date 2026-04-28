#ifndef LEARNWEBGPU_UTILITYHELPER_H
#define LEARNWEBGPU_UTILITYHELPER_H

#include <cassert>
#include <iostream>
#include <webgpu/webgpu.h>

inline WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    wgpuInstanceRequestAdapter(instance, options,
        [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* pUserData) {
            UserData& userDataRef = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestAdapterStatus_Success)
                userDataRef.adapter = adapter;
            else
                std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
            userDataRef.requestEnded = true;
        },
        &userData
    );

    #ifdef WEBGPU_BACKEND_DAWN
        while (!userData.requestEnded)
            wgpuInstanceProcessEvents(instance);
    #endif

    assert(userData.requestEnded);
    return userData.adapter;
}

inline WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    wgpuAdapterRequestDevice(adapter, descriptor,
        [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* pUserData) {
            UserData& userDataRef = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestDeviceStatus_Success)
                userDataRef.device = device;
            else
                std::cout << "Could not get WebGPU device: " << message.data << std::endl;
            userDataRef.requestEnded = true;
        },
        &userData
    );

    #ifdef WEBGPU_BACKEND_DAWN
        while (!userData.requestEnded)
            wgpuInstanceProcessEvents(instance);
    #endif

    assert(userData.requestEnded);
    return userData.device;
}

inline void inspectDevice(WGPUDevice device) {
    WGPUSupportedFeatures supported = {};
    wgpuDeviceGetFeatures(device, &supported);

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (size_t i = 0; i < supported.featureCount; ++i)
        std::cout << " - 0x" << supported.features[i] << std::endl;
    std::cout << std::dec;

    WGPUSupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = wgpuDeviceGetLimits(device, &limits);
    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
    }
}

#endif //LEARNWEBGPU_UTILITYHELPER_H