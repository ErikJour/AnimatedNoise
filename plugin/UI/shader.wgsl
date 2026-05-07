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
    let wave = sin(u.time * u.frequency) * u.amplitude;
    let ratio = 800.0 / 450.0;
    let offset = vec2f(-0.6875, -0.463);
    let angle = u.time * 0.5;
    let alpha = cos(angle);
    let beta = sin(angle);
    var position = vec3f(
        in.position.x + alpha,
        alpha * in.position.y + beta * in.position.z,
        alpha * in.position.z - beta * in.position.y,
    );
    out.position = vec4f(position.x, position.y * ratio, position.z * 0.5 + 0.5, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let wave = sin(u.time * u.frequency) * u.amplitude;
    let linear_color = pow(in.color + wave * 0.05, vec3f(2.2));

    return vec4f(linear_color, 1.0);

}