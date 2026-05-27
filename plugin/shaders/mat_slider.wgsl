// mat_slider.wgsl
const PANEL_ASPECT : f32 = 0.22;
const TRACK_HALF_W : f32 = 0.04;

// ── Vertex ───────────────────────────────────────────────────────────────────
fn vsSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

// ── Fragment ─────────────────────────────────────────────────────────────────
fn shadeSlider(in: VertexOutput) -> vec4f {
    let uv = in.color.xy;

    // --- EUGENE TSUI TRACK: The Spinal Column ---
    // 1. Undulation: Track weaves slightly left and right
    let spineWobble = sin(uv.y * 31.415) * 0.015; // ≈ 10*PI
    let trackCenter = 0.5 + spineWobble;
    // 2. Tapering: Stem is thicker at the bottom, narrowing at the top
    let dynamicWidth = TRACK_HALF_W * (1.5 - uv.y * 0.8);
    let baseTrackSDF = abs(uv.x - trackCenter) - dynamicWidth;
    // 3. Cellular Segmentation: Carve ridges into the track
    let segments = sin(uv.y * 120.0) * 0.008;
    let trackSDF = baseTrackSDF + segments;
    // The distance field is now just the track itself
    let d = trackSDF;
    let alpha = smoothstep(0.015, -0.005, d) * 0.85;
    // --- CARAPACE VIGNETTE ---
    // Wavy, irregular edge framing
    let edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    let wavyEdge = edge + sin(uv.y * 25.0) * 0.015;
    let vignette = smoothstep(0.0, 0.12, wavyEdge);
    // --- BIOMIMETIC COLORS ---
    let filled   = uv.y < u.sliderValue;
    // Deep moss/abyssal green for the empty carved channel
    let groove   = vec3f(0.06, 0.12, 0.08);
    // Bioluminescent teal/cyan for the filled fluid
    let filledC  = vec3f(0.15, 0.65, 0.55) * 0.9;
    // Base color selection (notch removed)
    var base = select(groove, filledC, filled);
    // Add a cellular interior glow to the fluid track
    if (filled) {
        let veinPulse = (sin(uv.y * 60.0) * 0.5 + 0.5) * 0.2;
        base = base + vec3f(0.1, 0.3, 0.2) * veinPulse;
        // Optional: Add a slight brightness "cap" at the very top of the fluid
        let meniscus = smoothstep(0.02, 0.0, u.sliderValue - uv.y);
        base = base + (vec3f(0.2, 0.8, 0.6) * meniscus * 0.5);
    }

    return vec4f(base * vignette, alpha);
}