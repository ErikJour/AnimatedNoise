struct Uniforms {
    time: f32,
    frequency: f32,
    amplitude: f32,
    _pad: f32
};

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@group(0) @binding(0) var<uniform> u: Uniforms;

const PI: f32 = 3.14159265359;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    var position = in.position;
    let angle1 = u.time;
    let c1 = cos(angle1);
    let s1 = sin(angle1);
    let R1 = transpose(mat4x4f(
         c1,  s1, 0.0, 0.0,
        -s1,  c1, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ));
    let angle2 = 3.0 * PI / 4.0;
    let c2 = cos(angle2);
    let s2 = sin(angle2);
    let R2 = transpose(mat4x4f(
        1.0, 0.0, 0.0, 0.0,
        0.0,  c2,  s2, 0.0,
        0.0, -s2,  c2, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ));
    // Scale the object
    let S = transpose(mat4x4f(
        0.3,  0.0, 0.0, 0.0,
        0.0,  0.3, 0.0, 0.0,
        0.0,  0.0, 0.3, 0.0,
        0.0,  0.0, 0.0, 1.0,
    ));

    // Translate the object
    let T = transpose(mat4x4f(
        1.0,  0.0, 0.0, 0.5,
        0.0,  1.0, 0.0, 0.0,
        0.0,  0.0, 1.0, 0.0,
        0.0,  0.0, 0.0, 1.0,
    ));
    let homogeneous_position = vec4f(position, 1.0);
    position = (R2 * R1 * T * S * homogeneous_position).xyz;
    out.position = vec4f(position.x, position.y, position.z * 0.5 + 0.5, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);

}