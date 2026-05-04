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

#==================================
#Cpp file locations
#==================================
target_sources(AnimatedNoise
        PRIVATE
        source/PluginEditor.cpp
        source/PluginProcessor.cpp
        UI/webGpuWindow.cpp
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
)

#==================================
#Link libraries
#==================================
target_link_libraries(AnimatedNoise
        PRIVATE
            webgpu
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

#==================================
# Shader file
#==================================
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/UI/shader.wgsl" SHADER_SOURCE)
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/UI/shader.wgsl.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/shader.wgsl.h"
)
target_include_directories(AnimatedNoise PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")