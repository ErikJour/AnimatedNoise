@group(0) @binding(0) var logoTex: texture_2d<f32>;
@group(0) @binding(1) var logoSmp: sampler;

fn shadeLogo(in: VertexOutput) -> vec4f {
//    let lum   = textureSample(logoTex, logoSmp, in.uv).r;
//    let alpha = smoothstep(0.15, 0.65, lum) * 0.75;
    return vec4f(1.0, 1.0, 1.0, 1.0);
}

fn vsLogo(pos: vec3f) -> vec4f {
        return u.projMatrix * u.modelMatrix * vec4f(pos, 1.0);

}