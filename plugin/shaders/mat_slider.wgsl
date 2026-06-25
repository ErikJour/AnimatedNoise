// ── Shared bioluminescent tube shader — used by all four slider materials ─────
//
// in.color.xy = (u, v)  — u around tube, v = normalised height [0..1]
// in.color.z  = pad     — 0: spine tube,  1: indicator ring
// in.normal   = tube surface normal (outward from spine centre)
//
fn shadeSpineTube(in: VertexOutput) -> vec4f {
    let cream      = vec3f(0.92, 0.86, 0.72);   // filled / below value
    let grey       = vec3f(0.28, 0.27, 0.25);   // empty  / above value
    let v          = in.color.y;
    let isIndicator = in.color.z > 0.5;

    // ── Indicator ring branch ─────────────────────────────────────────────────
    // Wider tube, only visible near sliderValue. Paints the FULL circumference so
    // the bead wraps all the way around the tendril — depth-testing hides the far
    // side, so it reads as a glowing ring you can see (and grab) from any angle.
    if (isIndicator) {
        let halfH = 0.048;
        // Clamp the indicator centre so it always has a full halfH of geometry on both sides.
        // Without this, the half of the indicator that would fall outside [0,1] has no
        // geometry to render on and disappears at the top and bottom of the slider range.
        let indicatorCenter = clamp(u.sliderValue, halfH, 1.0 - halfH);
        let dCenter = abs(v - indicatorCenter);        // distance from bead centre in v
        if (dCenter > halfH) { discard; }

        // Bead alpha — brightest at the value height, fading out toward the rounded
        // cap edges. No front-face gating: the whole ring around the tube lights up.
        let baseAlpha = smoothstep(halfH, 0.0, dCenter);
        var alpha     = baseAlpha * 0.95;
        if (alpha < 0.01) { discard; }

        // Gentle rim so the ring still reads as a rounded 3D surface as it wraps.
        let rim = 1.0 - abs(in.normal.y);

        let pulse         = sin(u.time * 4.0) * 0.15 + 0.85;
        let restColor     = vec3f(1.0, 0.38, 0.06);
        let pressedColor  = vec3f(1.0, 0.82, 0.50);   // bright warm-white when held
        let indicatorColor = mix(restColor, pressedColor, u.pressed) * pulse
                           * (0.85 + 0.15 * rim * rim);
        let pressedAlpha  = mix(alpha, min(alpha * 1.25, 1.0), u.pressed);
        return vec4f(indicatorColor, pressedAlpha);
    }

    // ── Spine tube branch ─────────────────────────────────────────────────────

    // Sharp fill boundary — tight transition for a crisp rising-fluid look
    let fillEdge = smoothstep(u.sliderValue - 0.03, u.sliderValue + 0.01, v);

    // Animated bioluminescent pulse only below fill level
    let pulse     = sin(u.time * 2.0 + v * 6.0) * 0.15 + 0.85;
    let pulseMask = 1.0 - step(u.sliderValue, v);
    let animPulse = mix(1.0, pulse, pulseMask);

    // Cream below fill level, grey above
    let creamWithPulse = cream * animPulse * 0.90;
    var finalColor     = mix(creamWithPulse, grey, fillEdge);

    // Rim highlight on the filled section
    let rim = 1.0 - abs(in.normal.y);
    finalColor += cream * (rim * rim) * 0.18 * (1.0 - fillEdge);

    // Meniscus: bright cream edge right at the fluid surface.
    // step(0.0, meniscusDist) gates it to zero above the fill level —
    // without it, the inverted smoothstep returns 1 for all v > sliderValue,
    // which floods the grey zone with cream.
    let meniscusDist = u.sliderValue - v;
    let meniscus     = smoothstep(0.025, 0.0, meniscusDist) * step(0.0, meniscusDist);
    finalColor      += cream * meniscus * 0.60;

    // Alpha: opaque while glowing, more transparent when dark/empty
    let alpha = mix(0.90, 0.35, fillEdge);

    return vec4f(finalColor, alpha);
}

// ── Noise Level (Gain) Slider — teal ─────────────────────────────────────────
fn vsGainSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

fn shadeGainSlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}
