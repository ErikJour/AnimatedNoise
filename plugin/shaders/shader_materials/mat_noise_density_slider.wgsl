// ── Noise Density Slider — green-teal ────────────────────────────────────────
fn vsDensitySlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

fn shadeDensitySlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}