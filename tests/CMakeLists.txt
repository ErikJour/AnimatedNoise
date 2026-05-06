add_executable(AnimatedNoiseTests
    uniform_tests.cpp
)

set_target_properties(AnimatedNoiseTests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(AnimatedNoiseTests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/plugin/UI
)

target_link_libraries(AnimatedNoiseTests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(AnimatedNoiseTests)
