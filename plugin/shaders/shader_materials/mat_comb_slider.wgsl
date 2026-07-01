// ── Comb Amount Slider — purple ───────────────────────────────────────────────
fn vsCombSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

fn shadeCombSlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}
