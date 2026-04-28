#==================================
#Animated Root-level CMake
#==================================
cmake_minimum_required(VERSION 4.0.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


project(
        AnimatedNoise
        VERSION 0.1.0
        LANGUAGES CXX C
)


add_subdirectory(libs/JUCE)
add_subdirectory(webgpu)
add_subdirectory(plugin)


