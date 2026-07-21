// ── Noise Density Slider — green-teal ────────────────────────────────────────
fn vertexDensitySlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    let expandedRatio = (u.pressed * 0.13) + 1.0 ;

    let expanded = vec3f(
                pos.x * expandedRatio,
                pos.y * expandedRatio,
                pos.z * expandedRatio
    );
    let sliderPosition = u.sliderPosition;
    let worldPosition  = vec4f(expanded + sliderPosition, 1.0);
    *pos               = worldPosition.xyz;

    return projectPerspective(worldPosition.xyz);
}

fn fragmentDensitySlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}