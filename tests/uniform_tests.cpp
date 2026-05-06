#include <catch2/catch_test_macros.hpp>
#include "MyUniforms.h"

TEST_CASE("MyUniforms layout satisfies WebGPU 16-byte alignment") {
    STATIC_REQUIRE(sizeof(MyUniforms) % 16 == 0);
}

TEST_CASE("MyUniforms field offsets match WGSL struct layout") {
    STATIC_REQUIRE(offsetof(MyUniforms, time)      == 0);
    STATIC_REQUIRE(offsetof(MyUniforms, frequency) == 4);
    STATIC_REQUIRE(offsetof(MyUniforms, amplitude) == 8);
    STATIC_REQUIRE(offsetof(MyUniforms, _pad)      == 12);
}
