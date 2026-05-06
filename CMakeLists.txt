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


include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.0
)
FetchContent_MakeAvailable(Catch2)

add_subdirectory(libs/JUCE)
add_subdirectory(webgpu)
add_subdirectory(plugin)

enable_testing()
add_subdirectory(tests)


