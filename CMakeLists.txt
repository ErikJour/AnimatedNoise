cmake_minimum_required(VERSION 4.0.0)

project(
        LearnWebGPU
        VERSION 0.1.0
        LANGUAGES CXX C
)

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

if (MACOS)
    set_target_properties(AnimatedNoise PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

add_subdirectory(webgpu)

target_link_libraries(AnimatedNoise PRIVATE webgpu)

target_copy_webgpu_binaries(AnimatedNoise)
