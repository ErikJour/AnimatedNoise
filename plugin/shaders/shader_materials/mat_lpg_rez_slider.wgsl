// ── LPG Resonance Slider — magenta ───────────────────────────────────────────
fn vsLpgRezSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
        let sliderPosition = u.sliderPosition;
        let worldPosition = vec4f(*pos + sliderPosition, 1.0);
        *pos = worldPosition.xyz;
        return projectPerspective(worldPosition.xyz);
}

fn shadeLpgRezSlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}
