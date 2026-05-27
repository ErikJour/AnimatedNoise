// mat_slider.wgsl
// Painted-on-wall slider. UV arrives packed in @location(1).xy:
//   u = 0..1 across arc  (left → right = 0 → max)
//   v = 0..1 up panel    (bottom → top)

// Panel world dimensions — match buildSliderGeometry params
// width  = 2 * halfSpan * wallRadius = 2 * 0.30 * 0.93 ≈ 0.558
// height = yTop - yBottom            = 0.20 - (-0.05)  = 0.25
const PANEL_ASPECT : f32 = 0.22;   // width/height — now tall and narrow
const TRACK_HALF_H : f32 = 0.08;   // in v-space
const TRACK_HALF_W : f32 = 0.08;   // ← add this
const THUMB_RADIUS : f32 = 0.12;   // in v-space (≈ 0.03 world units)

// ── Vertex ───────────────────────────────────────────────────────────────────
fn vsSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    // Geometry is baked to wall — no sliderPos offset, no manual projection
    return u.viewProjMatrix * vec4f(*pos, 1.0);
}

// ── Fragment ─────────────────────────────────────────────────────────────────
fn shadeSlider(in: VertexOutput) -> vec4f {
    let uv = in.color.xy;

    // Track — thin groove
    let trackSDF = abs(uv.x - 0.5) - TRACK_HALF_W;


    // Notch — horizontal rectangle, not a circle
    let notchSDF = abs(uv.y - u.sliderValue) - 0.022;


    let d = min(trackSDF, notchSDF);
    let alpha = smoothstep(0.015, -0.005, d) * 0.75;

    // Edge vignette — darkens toward panel border, sells the recess
    let edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    let vignette = smoothstep(0.0, 0.08, edge);

    // Colors
    let filled   = uv.y < u.sliderValue;
    let groove   = vec3f(0.15, 0.12, 0.10);          // dark carved channel
    let filledC  = vec3f(0.65, 0.35, 0.37) * 0.85;   // muted fill
    let notchC   = vec3f(0.80, 0.48, 0.50);           // slightly brighter cap

    let onNotch = notchSDF < 0.0;  // was: notchSDF < trackSDF
    let base     = select(select(groove, filledC, filled), notchC, onNotch);

    return vec4f(base * vignette, alpha);
}