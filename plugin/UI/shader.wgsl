struct Uniforms {
    time: f32,
    frequency: f32,
    amplitude: f32,
    _pad: f32
};

struct VertexInput {
    @location(0) position: vec2f,
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
    let ratio = 800.0 / 450.0; //w / h
    let offset = vec2f(-0.6875, -0.463); // The offset that we want to apply to the position
    out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let wave = sin(u.time * u.frequency) * u.amplitude;
    let linear_color = pow(in.color, vec3f(2.2));

    return vec4f(linear_color, 1.0);

}