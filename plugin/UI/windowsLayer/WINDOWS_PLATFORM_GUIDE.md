# Windows Platform Layer Guide

A plain-English guide to building the Windows version of the GPU surface, the
counterpart to `UI/macOSLayer/GpuSurface.mm`.

---

## The big picture

On macOS, we make an `NSView` with a `CAMetalLayer`, give the layer to WebGPU so
it has something to draw into, and hand the view to JUCE's `NSViewComponent`,
which places it inside the plugin window.

Windows works the same way, with different names:

| Job                             | macOS                  | Windows                                  |
|---------------------------------|------------------------|------------------------------------------|
| The thing we draw into          | `NSView` + `CAMetalLayer` | A child window (`HWND`)               |
| How WebGPU sees it              | `WGPUSurfaceSourceMetalLayer` | `WGPUSurfaceSourceWindowsHWND`   |
| How JUCE hosts it               | `juce::NSViewComponent` | `juce::HWNDComponent`                   |
| Letting mouse clicks fall through | `hitTest:` returns `nil` | `WM_NCHITTEST` returns `HTTRANSPARENT` |
| GPU backend underneath          | Metal                  | Vulkan (see "Things to know" below)      |

Everything else (the scene, shaders, sliders, and the timer and render loop) is
already platform-neutral and doesn't change.

### How Handmade Hero fits in

Handmade Hero's Win32 layer **owns the whole program**: it has `WinMain`, its own
top-level window, its own message loop, and it blits pixels with GDI. A plugin is
different. **The DAW owns the program and the message loop, and JUCE owns the
plugin window.** So:

- **Use from Handmade Hero:** how window classes, `CreateWindowEx`, `WndProc`,
  and window messages work.
- **Skip from Handmade Hero:** `WinMain`, the `PeekMessage` loop, back buffers,
  and `StretchDIBits`. WebGPU replaces the drawing part.

---

## Step by step

### Step 1: Make the header platform-neutral

Move `UI/macOSLayer/GpuSurface.h` up to `UI/GpuSurface.h` so both platforms share
it. Remove the `#if defined(__APPLE__)` guard and rename things so they don't say
"Metal":

```cpp
struct NativeSurface
{
    WGPUSurface surface = nullptr;
    void*       view    = nullptr;   // NSView* on macOS, HWND on Windows
};

NativeSurface createNativeSurface(WGPUInstance instance, double contentsScale);
```

Update `GpuSurface.mm` to use the new names. It should still build and run on the
Mac before you go any further.

### Step 2: Register a window class (once)

Before Windows lets you create a window, you have to describe a "class" of window:
mainly which function handles its messages. Handmade Hero does this on Day 002.

Two plugin-specific rules:

1. **Use your DLL's handle, not the host's.** Get it with
   `juce::Process::getCurrentModuleInstanceHandle()`. Handmade Hero gets it from
   `WinMain`, which a plugin doesn't have.
2. **Give the class a unique name.** A DAW can load two copies of your plugin, and
   Windows gets confused if both register the same class name. JUCE appends a
   timestamp for this reason, and you can do the same.

Only register the class once. Keep a static flag or the `ATOM` it returns.

### Step 3: Write the message handler (`WndProc`)

This is the function Windows calls whenever something happens to your window. It
only needs to handle three things:

- `WM_NCHITTEST`: return `HTTRANSPARENT`. This tells Windows "I'm not here for
  mouse purposes," so clicks and drags reach the JUCE editor underneath, where
  your slider and camera code already listens. It does the same job as
  `hitTest:` returning `nil` in the Mac version.
- `WM_ERASEBKGND`: return `1`. This stops Windows from painting a background over
  the GPU image, which would cause flicker.
- Everything else: pass it to `DefWindowProcW`.

### Step 4: Create the window

Call `CreateWindowExW` with your class name. Create it **as a popup with no
parent, size 1×1**:

```
style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
```

That looks odd, but it's intentional. When you later hand this window to JUCE's
`HWNDComponent`, JUCE changes it from a popup to a child window and parents it to
the plugin window itself. You don't need to know the parent when you create it,
the same way the Mac version creates its `NSView` without knowing where it'll go.

### Step 5: Create the WebGPU surface from the window

Fill in a `WGPUSurfaceSourceWindowsHWND`, which is the Windows version of
`WGPUSurfaceSourceMetalLayer`:

- `chain.sType = WGPUSType_SurfaceSourceWindowsHWND`
- `hinstance` = the same module handle from Step 2
- `hwnd` = the window from Step 4

Chain it into a `WGPUSurfaceDescriptor` and call `wgpuInstanceCreateSurface`,
exactly as the Mac file does. Return `{ surface, hwnd }`.

Put Steps 2–5 in `UI/windowsLayer/GpuSurface.cpp`. At the top, before including
`<windows.h>`:

```cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
```

The first keeps the header small. The second stops Windows from defining `min` and
`max` macros that break `std::min`/`std::max`. That matters because this project
treats warnings as errors.

### Step 6: Update `webGpuWindow.cpp`

Change the include to the shared header and call `createNativeSurface` instead of
`createMetalSurface`. No `#ifdef` is needed here, because the linker picks up
whichever platform file CMake compiled.

### Step 7: Host the window in the editor

In `include/AnimatedNoiseEditor.h`, next to the Mac member:

```cpp
#if JUCE_MAC
    juce::NSViewComponent mMetalView;
#elif JUCE_WINDOWS
    juce::HWNDComponent   mNativeView;
#endif
```

In `source/AnimatedNoiseEditor.cpp`, where the comment says
`//Erik -> Add if Windows here`, repeat what the Mac block does, using
`setHWND(...)` instead of `setView(...)`:

1. `setInterceptsMouseClicks(false, false)`
2. `addAndMakeVisible(...)`
3. `setHWND(mWebGpuWindow.getNativeView())`
4. `setBounds(getLocalBounds())`

Also add the `setBounds(getLocalBounds())` line in `resized()`.

### Step 8: Tell CMake about the new file

In `plugin/CMakeLists.txt`, next to the `if (APPLE)` block that adds
`GpuSurface.mm`:

```cmake
elseif (WIN32)
    target_sources(AnimatedNoise PRIVATE UI/windowsLayer/GpuSurface.cpp)
```

No extra libraries are needed. `user32` is linked by default.

### Step 9: Cleanup

**Important:** JUCE's `HWNDComponent` *takes ownership* of the window. When it's
destroyed or given a different `HWND`, it calls `DestroyWindow` for you. So:

- **Don't** call `DestroyWindow` yourself, or it will be destroyed twice.
- **Do** release the `WGPUSurface` before the editor goes away. It's safest to do
  this before the window is destroyed, so release it in the editor's destructor
  before `mNativeView` is torn down.
- Optionally call `UnregisterClassW` when the last instance closes. JUCE only
  unregisters when no windows still use the class.

### Step 10: Build and test on Windows

Test in at least two hosts (for example Reaper and Ableton) and check:

- The scene appears and fills the plugin window.
- Sliders and camera dragging work. If they don't, `WM_NCHITTEST` isn't returning
  `HTTRANSPARENT`.
- Resizing the window keeps the image sharp and correctly sized.
- Moving the window between two monitors with different scaling (e.g. 100% and
  150%) still looks right.
- Opening two instances of the plugin at once works (this tests the unique class
  name).
- Closing and reopening the editor doesn't crash (this tests cleanup order).

---

## Things to know about this project specifically

- **The GPU backend on Windows is Vulkan, not DirectX.** `webgpu/cmake/FetchDawn.cmake`
  sets `DAWN_ENABLE_D3D12 OFF` and turns Vulkan on for anything that isn't Apple.
  Almost all modern Windows GPUs support Vulkan, but D3D12 is the more native
  option. If you run into driver problems, consider turning on
  `DAWN_ENABLE_D3D12` for `WIN32` in that file.
- **No WebGPU DLL to ship.** Dawn is statically linked, and
  `target_copy_webgpu_binaries` is intentionally empty, so there's nothing to copy
  next to the `.vst3`.
- **Dawn builds from source.** Your first Windows build will download and compile
  Dawn, which is slow and needs **Git** and **Python** on the `PATH`.
- **Display scale.** The editor currently uses the *primary* display's scale.
  On Windows, where every monitor can have its own scale, it's more accurate to
  use `getPeer()->getPlatformScaleFactor()`, which is what `HWNDComponent` itself
  uses.
- **AU is Mac-only.** `FORMATS AU VST3 Standalone` is fine, because JUCE skips AU
  on Windows automatically.

---

## Resources

### In this repo (JUCE source): read these first

- **`libs/JUCE/modules/juce_gui_extra/native/juce_HWNDComponent_windows.cpp`**
  The whole Windows version of `NSViewComponent`, about 150 lines. Read
  `addToParent()` (popup → child, `SetParent`), `componentMovedOrResized()`
  (scaling and `SetWindowPos`), and `~Pimpl()` (`DestroyWindow`, which is why you
  don't call it yourself).
- **`libs/JUCE/modules/juce_gui_extra/embedding/juce_HWNDComponent.h`**
  The public API: `setHWND`, `getHWND`, `updateHWNDBounds`.
- **`libs/JUCE/modules/juce_gui_basics/native/juce_Windowing_windows.cpp`**
  - Lines ~2092–2135, `WindowClassHolder`: how JUCE registers a window class
    safely from a DLL (module handle and unique name).
  - Line ~3673, `WM_NCHITTEST` → `HTTRANSPARENT`: JUCE's own click-through.
- **`libs/JUCE/modules/juce_opengl/native/juce_OpenGL_windows.h`**
  - `createNativeWindow()` (~line 344) and `updateWindowPosition()` (~line 158):
    how JUCE embeds a GPU-rendering child window in a component on Windows. This
    is the closest thing in JUCE to what we're building. Note how it wraps window
    work in `ScopedThreadDPIAwarenessSetter`.

### Handmade Hero

- Episode guide: https://guide.handmadehero.org/
- **Day 001: Setting Up the Windows Build**: compiler, debugger, and Visual Studio basics.
- **Day 002: Opening a Win32 Window**: `WNDCLASS`, `RegisterClass`,
  `CreateWindowEx`, `WndProc`. This is the core material for Steps 2–4.
- Day 003 onward (back buffers, GDI blitting) is useful background, but
  WebGPU replaces it here.

### Microsoft documentation

- Learn to program for Windows (a gentle Win32 intro):
  https://learn.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows
- `WNDCLASSEXW`:
  https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexw
- `CreateWindowExW`:
  https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw
- `WM_NCHITTEST`:
  https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest
- High-DPI desktop apps:
  https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows

### WebGPU

- Learn WebGPU for C++ (the guide this project's `webgpu/` folder comes from):
  https://eliemichel.github.io/LearnWebGPU/
- `glfw3webgpu`, a small file that creates a WebGPU surface from an `HWND` (and
  from a Metal layer). It's a good side-by-side check for Step 5:
  https://github.com/eliemichel/glfw3webgpu
