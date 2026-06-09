//===============================
//Vertex
//===============================
fn vsDensitySlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

//===============================
//Fragment
//===============================
fn shadeDensitySlider(in: VertexOutput) -> vec4f {
    let uv = in.color.xy;

    //===============================
    //Undulation
    //===============================
    let spineWobble = sin(uv.y * 31.415) * 0.015;
    let trackCenter = 0.5 + spineWobble;
    //===============================
    //Tapering
    //===============================
    let dynamicWidth = TRACK_HALF_W * (1.5 - uv.y * 0.8);
    let baseTrackSDF = abs(uv.x - trackCenter) - dynamicWidth;
    //===============================
    //Maybe remove this
    //===============================
    let segments = sin(uv.y * 120.0) * 0.008;
    let trackSDF = baseTrackSDF + segments;
    //===============================
    //Distance
    //===============================
    let d = trackSDF;
    let alpha = smoothstep(0.015, -0.005, d) * 0.85;
    //===============================
    //Waviness
    //===============================
    let edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    let wavyEdge = edge + sin(uv.y * 25.0) * 0.015;
    let vignette = smoothstep(0.0, 0.12, wavyEdge);
    //===============================
    //Colors
    //===============================
    let filled   = uv.y < u.sliderValue;
    // Moss green for the empty channel
    let groove   = vec3f(0.06, 0.12, 0.08);
    // Teal for the filled fluid
    let filledC  = vec3f(0.15, 0.65, 0.55) * 0.9;
    // Base color
    var base = select(groove, filledC, filled);
    // Interior glow
    if (filled) {
        let veinPulse = (sin(uv.y * 60.0) * 0.5 + 0.5) * 0.2;
        base = base + vec3f(0.1, 0.3, 0.2) * veinPulse;
        let meniscus = smoothstep(0.02, 0.0, u.sliderValue - uv.y);
        base = base + (vec3f(0.2, 0.8, 0.6) * meniscus * 0.5);
    }

    return vec4f(base * vignette, alpha);
}