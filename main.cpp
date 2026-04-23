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
WGPUInstance mInstance = nullptr;
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

#ifdef WEBGPU_BACKEND_DAWN
    WGPUDawnTogglesDescriptor toggles;
    toggles.chain.next = nullptr;
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.disabledToggleCount = 0;
    toggles.enabledToggleCount = 1;
    const char* toggleName = "enable_immediate_error_handling";
    toggles.enabledToggles = &toggleName;

    descriptor.nextInChain = &toggles.chain;
#endif

    mInstance = wgpuCreateInstance(&descriptor);

    if (!mInstance)
    {
        std::cerr << "Failed to create WGPU instance." << std::endl;
        return 1;
    }

    std::cout << "Created WGPU instance." << mInstance << std::endl;

    //=================================================================
    //Get the adapter
    //=================================================================
    std::cout << "Requesting adapter..." << std::endl;
    adapterOpts.nextInChain = nullptr;
    const WGPUAdapter mAdapter = requestAdapterSync(mInstance, &adapterOpts);
    getAdapter(mAdapter, initProperties);
    //=================================================================
    // Query adapter BEFORE releasing it
    //=================================================================
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
    //Descriptor
    //=================================================================
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, char const* message, void*) {
        if (reason == WGPUDeviceLostReason_Destroyed) return;
        std::cerr << "Unexpected device lost: reason " << reason;
        if (message) std::cerr << " (" << message << ")";
        std::cerr << std::endl;
    };
    deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    //=================================================================
    //Device
    //=================================================================
    std::cout << "Requesting device..." << std::endl;
    WGPUDevice mDevice = requestDeviceSync(mAdapter, &deviceDesc);
    std::cout << "Got device: " << mDevice << std::endl;

    wgpuAdapterRelease(mAdapter); //You can release the adapter at this point
    inspectDevice(mDevice);

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(mDevice, onDeviceError, nullptr /* pUserData */);
    //=================================================================
    //Queue
    //=================================================================
    WGPUQueue mQueue = wgpuDeviceGetQueue(mDevice);
    //Example data send from cpu to gpu
    // WGPUCommandBuffer commands[3];
    // wgpuQueueSubmit(mQueue, 3, commands);
    //=================================================================
    //Command Encoder
    //=================================================================
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder); // release encoder after it's finished


    bool workDone = false;
    WGPUQueueWorkDoneCallbackInfo2 callbackInfo = {};
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, void* pUserData, void*) {
        std::cout << "Queued work finished with status: " << status << std::endl;
        *reinterpret_cast<bool*>(pUserData) = true;
    };
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.userdata1 = &workDone;
    wgpuQueueOnSubmittedWorkDone2(mQueue, callbackInfo);

    // Finally submit the command queue
    std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(mQueue, 1, &command);
    wgpuCommandBufferRelease(command);
    std::cout << "Command submitted." << std::endl;

    while (!workDone) {
        std::cout << "Ticking device..." << std::endl;
#ifdef WEBGPU_BACKEND_DAWN
        wgpuDeviceTick(mDevice);
#elif defined(WEBGPU_BACKEND_WGPU)
        wgpuDevicePoll(mDevice, false, nullptr);
#endif
    }

    //=================================================================
    // Release resources — adapter already released above, don't pass it
    //=================================================================
    std::cout << "Releasing Queue..." << std::endl;
    wgpuQueueRelease(mQueue);
    std::cout << "Releasing device..." << std::endl;
    wgpuDeviceRelease(mDevice);
    // std::cout << "Releasing Command..." << std::endl;
    // for (auto cmd : commands) {
    //     wgpuCommandBufferRelease(cmd);
    // }
    std::cout << "Releasing instance..." << std::endl;
    wgpuInstanceRelease(mInstance);

    return 0;
}

