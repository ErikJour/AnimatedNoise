# WebGPU / Dawn Session — April 21, 2026
## AnimatedNoise — Native GPU Renderer Foundation

---

## Session Summary

First session establishing native WebGPU rendering infrastructure for AnimatedNoise using Dawn on Apple M5 (Metal backend). Covered the full Instance → Adapter interrogation pipeline with clean lifecycle management, and established the architectural roadmap toward a triangle in a JUCE window.

---

## Concepts Covered

### Descriptors and Creation
WebGPU uses a **descriptor → factory → opaque handle** pattern for every object:
```cpp
WGPUInstanceDescriptor descriptor = {};
descriptor.nextInChain = nullptr;
WGPUInstance instance = wgpuCreateInstance(&descriptor);
```
- `nextInChain` is a linked list extension pointer, present on every descriptor
- Factory functions take a pointer to the descriptor, not a reference (C API)
- Returned handles are opaque — never touch internals directly

### Object Hierarchy
```
WGPUInstance          ← runtime context, root of ownership
    └── WGPUAdapter   ← physical hardware handle
        └── WGPUDevice ← logical connection + capability contract
            ├── WGPUQueue
            ├── WGPUBuffer
            ├── WGPURenderPipeline
            └── WGPUSurface
```

### Adapter vs Device — The Portability Contract
- **Adapter** — interrogates what hardware actually exists
- **Device** — locks in only the capabilities you explicitly declare
- Prevents "it worked on my machine" bugs across user hardware

### Async Callback Pattern
`wgpuInstanceRequestAdapter` is asynchronous — no WebGPU function blocks the CPU:
```cpp
struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
};

auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
    WGPUAdapter adapter, char const* message, void* pUserData) {
    UserData& userData = *reinterpret_cast<UserData*>(pUserData);
    if (status == WGPURequestAdapterStatus_Success)
        userData.adapter = adapter;
    userData.requestEnded = true;
};
```
- Lambda must be **non-capturing** (`[]`) to behave as a C function pointer
- `userdata` pointer passes context from call site into callback
- Dawn resolves synchronously in native builds — async is the API contract, not the behavior

### Lifecycle — Destructor Pattern
In a JUCE plugin, `main()` is replaced by the Editor constructor/destructor:
```cpp
// Constructor — top-down creation
instance = wgpuCreateInstance(&descriptor);
adapter  = requestAdapterSync(instance, &opts);
device   = requestDeviceSync(adapter, &deviceDesc);

// Destructor — bottom-up release
wgpuDeviceRelease(device);
wgpuAdapterRelease(adapter);
wgpuInstanceRelease(instance);
```

---

## Hardware Confirmed

**Development Mac — Apple M5**
```
Adapter name:    Apple M5
Backend:         0x5 (Metal)
Architecture:    common-3
Driver:          Metal driver on macOS 26.3.1
Adapter type:    0x2 (IntegratedGPU — unified memory, expected on Apple Silicon)
```

Key limits:
- `maxTextureDimension2D`: 16384
- `maxTextureDimension3D`: 2048 (volumetric noise fields viable)

Full chain: `C++ → Dawn → Metal → Apple M5`

**Windows Machine — ASUS ROG Zephyrus G16**
- Intel 13th Gen Core i7 + NVIDIA GeForce RTX 4070
- Backend will be D3D12 (not D3D11 like WebView2/WebGL)
- Separate VRAM — buffer transfer costs matter (unlike M5 unified memory)
- Two adapters present — will need to explicitly request high-performance GPU

---

## Why WebView Was Slower on the ROG

WebView2/Three.js chain:
```
JUCE → WebView2 process → Chromium → WebGL → D3D11 → RTX 4070
```
Problems:
- D3D11 on a GPU that wants D3D12
- IPC process boundary on every frame/event
- Chromium compositor scheduling
- Possible fallback to Intel integrated graphics

Native Dawn chain:
```
JUCE → Dawn → D3D12 → RTX 4070
```
Mouse lag, keyboard forwarding bugs, race conditions — gone.

---

## Audio Thread → Visual Communication

WebView bridge (TDS-01):
```
Audio thread → JUCE MessageManager → postMessage →
JS bridge → WebView2 process → JavaScript → Three.js
```

Native Dawn:
```
Audio thread → atomic<float> → render thread
```

Implementation pattern:
```cpp
// Audio thread writes (DSP)
std::atomic<float> modulationDepth;

// Render thread reads (GPU loop)
float depth = modulationDepth.load();
```
Same process, no serialization, no IPC. Latency: microseconds instead of milliseconds. Visuals can be sample-accurate.

---

## Current CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 4.0.0)
project(LearnWebGPU VERSION 0.1.0 LANGUAGES CXX C)

add_executable(AnimatedNoise main.cpp)

set_target_properties(AnimatedNoise PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(AnimatedNoise PRIVATE /w4)
else()
    target_compile_options(AnimatedNoise PRIVATE -Wall -Wextra -pedantic)
endif()

if (APPLE) # Note: was MACOS — incorrect CMake variable
    set_target_properties(AnimatedNoise PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

add_subdirectory(glfw)         # Before webgpu (Dawn provides its own otherwise)
add_subdirectory(glfw3webgpu)
add_subdirectory(webgpu)

target_link_libraries(AnimatedNoise PRIVATE webgpu glfw glfw3webgpu)
target_copy_webgpu_binaries(AnimatedNoise)
```

---

## Shading Language

**WGSL** — WebGPU Shading Language. Replaces GLSL from Three.js Journey.

```wgsl
@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(1.0, 0.5, 0.0, 1.0);
}
```

Concepts transfer directly from GLSL: `vec4`, UV coordinates, uniforms, fragment/vertex stages, noise functions. Syntax differs, mental model is identical.

---

## Roadmap

```
This week
├── Device request (next session)
├── Blank JUCE plugin + Dawn in CMakeLists
├── WGPUSurface from JUCE editor window handle (skip GLFW)
└── Triangle in JUCE window

Week 2
└── Noise shaders, WGSL experimentation

Week 3+
└── Atomic audio→render bridge
    └── Visuals responding to DSP state in real time
```

**GLFW note:** Only needed for standalone `main.cpp` learning. In the JUCE plugin, `getWindowHandle()` on the editor gives the native handle directly. GLFW is skipped entirely in production.

---

## JUCE Plugin Architecture

```cpp
class AnimatedNoiseEditor : public juce::AudioProcessorEditor {
public:
    AnimatedNoiseEditor(AnimatedNoiseProcessor& p)
        : AudioProcessorEditor(p) {
        // Instance → Adapter → Device (top-down)
    }

    ~AnimatedNoiseEditor() {
        // Pipeline → Device → Adapter → Instance (bottom-up)
        wgpuRenderPipelineRelease(pipeline);
        wgpuQueueRelease(queue);
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
    }

private:
    WGPUInstance       instance = nullptr;
    WGPUAdapter        adapter  = nullptr;
    WGPUDevice         device   = nullptr;
    WGPUQueue          queue    = nullptr;
    WGPURenderPipeline pipeline = nullptr;
};
```

WebGPU lives entirely in the **Editor**, not the Processor. Processor stays GPU-ignorant — same discipline as TDS-01.

---

*Session conducted April 21, 2026. Next session: device request + JUCE migration.*