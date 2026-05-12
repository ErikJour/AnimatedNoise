fn isIndicator(color: vec3f) -> bool {
    return color.r > 0.9 && color.g > 0.35 && color.g < 0.45 && color.b < 0.15;
}

fn vsSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    if isIndicator(color) {
        let centerY = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y);
        (*pos).y   += centerY;
    }
    (*pos) = *pos + u.sliderPos;
    return projectFlat(*pos);
}

fn shadeSlider(in: VertexOutput) -> vec4f {
    if isIndicator(in.color) {
        return vec4f(0.88, 0.52, 0.55, 1.0);
    }
    let fillY = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y) + u.sliderPos.y;
    let col   = select(
        vec3f(0.22, 0.18, 0.12),
        vec3f(0.75, 0.40, 0.42) * 0.9,
        in.worldPos.y < fillY
    );
    return vec4f(col, 1.0);
}