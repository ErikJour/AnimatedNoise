//
// Created by Erik Jourgensen on 4/28/26.
//

#ifndef ANIMATEDNOISE_WEBGPUWINDOW_H
#define ANIMATEDNOISE_WEBGPUWINDOW_H
#include <webgpu/webgpu.h>
#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class WebGpuWindow
{
    public:
        // Initialize everything and return true if it went all right
        bool initialize()
        {
            //=================================================================
            //Create webgpu instance
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
            }
            std::cout << "Created WGPU instance." << mInstance << std::endl;

            //=================================================================
            //Get the adapter
            //=================================================================
            std::cout << "Requesting adapter..." << std::endl;
            adapterOpts.nextInChain = nullptr;
            mAdapter = requestAdapterSync(mInstance, &adapterOpts);
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
            WGPUAdapterInfo properties = {};
            properties.nextInChain = nullptr;
            wgpuAdapterGetInfo(mAdapter, &properties);
            std::cout << "Adapter properties:" << std::endl;
            std::cout << " - vendorID: " << properties.vendorID << std::endl;
            if (properties.vendor.length > 0) {
                std::cout << " - vendorName: ";
                std::cout.write(properties.vendor.data, static_cast<std::streamsize>(properties.vendor.length));
                std::cout << std::endl;
            }
            if (properties.architecture.length > 0) {
                std::cout << " - architecture: ";
                std::cout.write(properties.architecture.data, static_cast<std::streamsize>(properties.architecture.length));
                std::cout << std::endl;
            }
            std::cout << " - deviceID: " << properties.deviceID << std::endl;
            if (properties.device.length > 0) {
                std::cout << " - name: ";
                std::cout.write(properties.device.data, static_cast<std::streamsize>(properties.device.length));
                std::cout << std::endl;
            }
            if (properties.description.length > 0) {
                std::cout << " - driverDescription: ";
                std::cout.write(properties.description.data, static_cast<std::streamsize>(properties.description.length));
                std::cout << std::endl;
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
            deviceDesc.label = WGPU_STR("My Device");
            deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
            deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
            deviceDesc.defaultQueue.nextInChain = nullptr;
            // deviceDesc.defaultQueue.label = "The default queue";
            deviceDesc.deviceLostCallbackInfo2.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
                if (reason == WGPUDeviceLostReason_Destroyed) return;
                std::cerr << "Unexpected device lost: reason " << reason;
                if (message.length > 0) std::cerr << " (" << message.data << ")";
                std::cerr << std::endl;
            };
            deviceDesc.deviceLostCallbackInfo2.mode = WGPUCallbackMode_AllowSpontaneous;
            deviceDesc.uncapturedErrorCallbackInfo2.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
                std::cout << "Uncaptured device error: type " << type;
                if (message.length > 0) std::cout << " (" << message.data << ")";
                std::cout << std::endl;
            };
            //=================================================================
            //Device
            //=================================================================
            std::cout << "Requesting device..." << std::endl;
            mDevice = requestDeviceSync(mInstance, mAdapter, &deviceDesc);
            std::cout << "Got device: " << mDevice << std::endl;

            wgpuAdapterRelease(mAdapter); //You can release the adapter at this point
            inspectDevice(mDevice);

            //=================================================================
            //Queue
            //=================================================================
            mQueue = wgpuDeviceGetQueue(mDevice);
            //Example data send from cpu to gpu
            // WGPUCommandBuffer commands[3];
            // wgpuQueueSubmit(mQueue, 3, commands);
            //=================================================================
            //Command Encoder
            //=================================================================
            WGPUCommandEncoderDescriptor encoderDesc = {};
            encoderDesc.nextInChain = nullptr;
            encoderDesc.label = WGPU_STR("My command encoder");
            WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);
            wgpuCommandEncoderInsertDebugMarker(encoder, WGPU_STR("Do one thing"));
            wgpuCommandEncoderInsertDebugMarker(encoder, WGPU_STR("Do another thing"));
            WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
            cmdBufferDescriptor.nextInChain = nullptr;
            cmdBufferDescriptor.label = WGPU_STR("Command buffer");
            WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
            wgpuCommandEncoderRelease(encoder); // release encoder after it's finished


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

            return true;
        }
        // Uninitialize everything that was initialized
        void terminate() const
        {
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
        }
        // Draw a frame and handle events
        void mainLoop()
        {
                while (!workDone) {
                    std::cout << "Ticking device..." << std::endl;
            #ifdef WEBGPU_BACKEND_DAWN
                    wgpuDeviceTick(mDevice);
            #elif defined(WEBGPU_BACKEND_WGPU)
                    wgpuDevicePoll(mDevice, false, nullptr);
            #endif
                }
        }
        // Return true as long as the main loop should keep on running
        bool isRunning();

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

        WGPUInstanceDescriptor descriptor = {};
        WGPUInstance mInstance = nullptr;
        WGPURequestAdapterOptions adapterOpts = {};
        WGPUAdapterInfo initProperties = {};
        WGPUAdapter mAdapter = nullptr;
        WGPUDevice mDevice = nullptr;
        WGPUQueue mQueue = nullptr;
        bool workDone = false;
};

#endif //ANIMATEDNOISE_WEBGPUWINDOW_H