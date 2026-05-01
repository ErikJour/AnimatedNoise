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

fn mod_glsl(x: f32, y: f32) -> f32 {
    return x - y * floor(x / y);
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

    //==============================================
    //Shader Code
    //==============================================
    var patternSeventeenX: f32 = abs(vUv.x - 0.5);
    var patternSeventeenY: f32 = abs(vUv.y - 0.5);
    var patternSeventeen: f32 = min(patternSeventeenX, patternSeventeenY);
    return vec4f(vec3f(patternSeventeen), 1.0);
}
)";

#endif //ANIMATEDNOISE_SHADER_H

//=====================================================
//Pattern 3 - Bright right side fading to black on left
//=====================================================
//let strength = vUv.x;
//=====================================================
//Pattern 4 - Bright on top fading to black on bottom
//=====================================================
//let strength = vUv.y;
//=====================================================
//Pattern 5 - Bright on bottom fading to black on top
//=====================================================
//let strength = 1.0 - vUv.y;
//=====================================================
//Pattern 6 - Fan
//=====================================================
//let strength = mod_glsl(vUv.y * 10.0, 1.5);
//=====================================================
//Pattern 7 - Jailbird
//=====================================================
//float fanTwo = mod(weight * 10.0, 1.0);
//fanTwo = step(0.9, fanTwo);
//=====================================================
//Pattern 8 - Skinny horizontal lines
//=====================================================
//let strength: f32 = mod_glsl(vUv.y * 10.0, 1.0);
//let fanThree: f32 = step(0.9, strength);
//=====================================================
//Pattern 10 - Skinny vertical lines
//=====================================================
//let strength: f32 = mod_glsl(vUv.x * 10.0, 1.0);
//let patternTen: f32 = step(0.9, strength);
//=====================================================
//Pattern 11 - Box grid
//=====================================================
// let strength: f32 = step(0.9, mod_glsl(vUv.y * 10.0, 1.0));
// let fanFour: f32 = step(0.9, mod_glsl(vUv.x * 10.0, 1.0));
// let combo: f32 = strength + fanFour;
//=====================================================
// Pattern 14 -Cool corners
//=====================================================
// let barX: f32 = step(0.4, mod_glsl(vUv.x * 10.0, 1.0));
// let barXTwo: f32 = step(0.8, mod_glsl(vUv.y * 10.0, 1.0)) * barX;
// let barY: f32 = step(0.4, mod_glsl(vUv.y * 10.0, 1.0));
// let barYTwo: f32 = step(0.8, mod_glsl(vUv.x * 10.0, 1.0)) * barY;
//
// let strength: f32 = barXTwo + barYTwo;
//=====================================================
// Pattern 15 - Plus signs
//=====================================================
// var barFifteenX: f32 = step(0.4, mod_glsl(vUv.x * 10.0 - 0.2, 1.0));
// barFifteenX *= step(0.8, mod_glsl(vUv.y * 10.0, 1.0));
// var barFifteenY: f32 = step(0.4, mod_glsl(vUv.y * 10.0 - 0.2, 1.0));
// barFifteenY *= step(0.8, mod_glsl(vUv.x * 10.0, 1.0));
//
// let patternFifteen: f32 = barFifteenX + barFifteenY;
//=====================================================
// Pattern 16 - Dark center brightening on either side
//=====================================================
// var patternSixteen: f32 = abs(vUv.x - 0.5);
//=====================================================
//=====================================================
// Pattern 17 - Shadow cross
//=====================================================
//var patternSeventeenX: f32 = abs(vUv.x - 0.5);
//var patternSeventeenY: f32 = abs(vUv.y - 0.5);
//var patternSeventeen: f32 = min(patternSeventeenX, patternSeventeenY);
// Pattern 32: The Star Pattern
//=====================================================
//let rotatedUv = rotate(vUv, PI * 0.25, vec2f(0.5));

//let lightUvX = vec2f(rotatedUv.x * 0.1 + 0.45, rotatedUv.y * 0.5 + 0.25);
//let lightX = 0.015 / distance(lightUvX, vec2f(0.5));

//let lightUvY = vec2f(rotatedUv.y * 0.1 + 0.45, rotatedUv.x * 0.5 + 0.25);
//let lightY = 0.015 / distance(lightUvY, vec2f(0.5));

//let thirtyTwo = lightX + lightY;