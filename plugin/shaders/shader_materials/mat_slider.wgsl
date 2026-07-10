// ── Noise Level (Gain) Slider — teal ─────────────────────────────────────────
fn shadeGainSlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}

fn vsGainSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}


