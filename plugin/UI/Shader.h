//
// Created by Erik Jourgensen on 4/30/26.
//

#ifndef ANIMATEDNOISE_SHADER_H
#define ANIMATEDNOISE_SHADER_H



inline auto shaderSource = R"(

const PI: f32 = 3.14159265359;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) vUv: vec2<f32>,
};

//====================================================================================
//Vertex
//====================================================================================
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
    var out: VertexOutput;
    var p = vec2f(0.0, 0.0);
    var uv = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
        uv = vec2f(0.0, 0.0); // Bottom-left
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
        uv = vec2f(1.0, 0.0); // Bottom-right
    } else {
        p = vec2f(0.0, 0.5);
        uv = vec2f(0.5, 1.0); // Top-center
    }
    out.position = vec4f(p, 0.0, 1.0);
    out.vUv = uv;
    return out;
}

//====================================================================================
//Fragment
//====================================================================================
fn random(st: vec2f) -> f32 {
    return fract(sin(dot(st, vec2f(12.9898, 78.233))) * 43758.543123);
}

fn rotate(uv: vec2f, rotation: f32, mid: vec2f) -> vec2f {
    let s = sin(rotation);
    let c = cos(rotation);
    let translated = uv - mid;
    return vec2f(
        c * translated.x - s * translated.y + mid.x,
        s * translated.x + c * translated.y + mid.y
    );
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let vUv = in.vUv;

    // Pattern 32: The Star Pattern
    let rotatedUv = rotate(vUv, PI * 0.25, vec2f(0.5));

    let lightUvX = vec2f(rotatedUv.x * 0.1 + 0.45, rotatedUv.y * 0.5 + 0.25);
    let lightX = 0.015 / distance(lightUvX, vec2f(0.5));

    let lightUvY = vec2f(rotatedUv.y * 0.1 + 0.45, rotatedUv.x * 0.5 + 0.25);
    let lightY = 0.015 / distance(lightUvY, vec2f(0.5));

    let thirtyTwo = lightX + lightY;

    return vec4f(vec3f(thirtyTwo), 1.0);
}
)";

#endif //ANIMATEDNOISE_SHADER_H