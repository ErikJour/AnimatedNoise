#==================================
#Animated Noise Plugin CMake
#==================================

#==================================
#Set platform export directories
#==================================
if(APPLE)
    set(VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
    set(AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
elseif(WIN32)
    set(VST3_COPY_DIR "$ENV{COMMONPROGRAMFILES}/VST3")
endif()

#==================================
#Binary assets
#==================================
juce_add_binary_data(AnimatedNoiseAssets
        NAMESPACE BinaryData
        HEADER_NAME BinaryData.h
        SOURCES
        Assets/ERIK-JOURGENSEN_ICON_B_SMALL.png
)

#==================================
#Add plugin
#==================================
juce_add_plugin(AnimatedNoise
        PLUGIN_NAME "Animated Noise"
        COMPANY_NAME AnimatedInstruments
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        PLUGIN_MANUFACTURER_CODE JOUR
        PLUGIN_CODE ANIN
        FORMATS AU VST3 Standalone
        COPY_PLUGIN_AFTER_BUILD TRUE
        VST3_COPY_DIR "${VST3_COPY_DIR}"
        AU_COPY_DIR "${AU_COPY_DIR}"
)

#==================================
#Compiler settings
#==================================
set_target_properties(AnimatedNoise PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(AnimatedNoise PRIVATE /W4)
else()
    target_compile_options(AnimatedNoise PRIVATE -Wall -Wextra -pedantic)
endif()

#==================================
#Bring in WebGpu
#==================================
target_copy_webgpu_binaries(AnimatedNoise)

if (APPLE)
    set_target_properties(AnimatedNoise PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
            XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )
endif()

if (APPLE)
    target_sources(AnimatedNoise PRIVATE source/GpuSurface.mm)
    target_link_libraries(AnimatedNoise PRIVATE "-framework QuartzCore" "-framework Metal")
endif()

#==================================
#Cpp file locations
#==================================
target_sources(AnimatedNoise
        PRIVATE
        source/AnimatedNoiseEditor.cpp
        source/AnimatedNoiseProcessor.cpp
        UI/webGpuWindow.cpp
        UI/ResourceManager.cpp
        UI/Scene.cpp
        UI/components/slider/SliderManager.cpp
        dsp/NoiseSynth.cpp
        dsp/NoiseGenerator.cpp
        dsp/NoiseVoice.cpp
        dsp/AnimatedGain.cpp
        UI/components/logo/AnimatedLogo.cpp
        UI/components/slider/sliderCatalog.cpp
)

if (APPLE)
    target_sources(AnimatedNoise PRIVATE source/GpuSurface.mm)
    target_link_libraries(AnimatedNoise PRIVATE "-framework QuartzCore" "-framework Metal")
endif()

#==================================
#Include directories
#==================================
target_include_directories(AnimatedNoise
        PRIVATE
        include
        source
        UI
        geometries
        shaders
        dsp
        utilities
        UI/components
        UI/components/logo
        UI/components/slider
        UI/components/camera
        UI/components/mouse
        UI/components/text
)

#==================================
#Link libraries
#==================================
target_link_libraries(AnimatedNoise
        PRIVATE
            webgpu
            AnimatedNoiseAssets
            juce::juce_audio_utils
            juce::juce_gui_extra
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
)

#==================================
#
#==================================
target_compile_definitions(AnimatedNoise
        PUBLIC
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

option(DEV_MODE "Set up development helper settings" ON)

if(DEV_MODE)
target_compile_definitions(AnimatedNoise PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/geometries"
)

else()
    # In release mode, we just load geometries relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(AnimatedNoise PRIVATE
            RESOURCE_DIR="geometries"
    )
endif()

#==================================
# Shader files (concatenated in order)
#==================================
set(SHADER_FILES
        "shaders/shader_materials/common.wgsl"
        "shaders/shader_materials/lighting.wgsl"
        "shaders/shader_materials/mat_text.wgsl"
        "shaders/shader_materials/mat_slider.wgsl"
        "shaders/shader_materials/mat_plane.wgsl"
        "shaders/shader_materials/vs_main.wgsl"
        "shaders/shader_materials/fs_main.wgsl"
)

set(SHADER_SOURCE "")
foreach(SHADER_FILE ${SHADER_FILES})
    file(READ "${SHADER_FILE}" SHADER_CHUNK)
    string(APPEND SHADER_SOURCE "${SHADER_CHUNK}\n")
endforeach()

configure_file(
        "shaders/shader.wgsl.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/shader.wgsl.h"
)

target_compile_definitions(AnimatedNoise PRIVATE
        DEBUG_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

target_include_directories(AnimatedNoise PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")