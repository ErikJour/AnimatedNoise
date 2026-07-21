// ── Stained Glass Skylight — Prairie School voronoi panes ──────────────────
// WGSL port of the Three.js back-wall portal shader.
// Drop these functions into your shader module and replace shadeSkylight.

// World-space placement of the skylight disc (lies in the XZ plane).
// Set these to match your skylight geometry. If your VertexOutput already
// carries a uv, you can skip the derivation and use in.uv directly.
const SKYLIGHT_CENTER : vec2f = vec2f(0.0, 0.0);
const SKYLIGHT_RADIUS : f32   = 4.0;

const BEYOND_COLOR  : vec3f = vec3f(0.45, 0.65, 0.82);  // sky beyond the glass
const GLASS_OPACITY : f32   = 0.80;

fn vertexSkylight(pos: ptr<function, vec3f>) -> vec4f {
    let worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    *pos = worldPos.xyz;
    return projectPerspective(worldPos.xyz);
}

// ── Helpers ─────────────────────────────────────────────────────────────────

fn hash21(p: vec2f) -> f32 {
    return fract(sin(dot(p, vec2f(127.1, 311.7))) * 43758.5453);
}

// WGSL can't dynamically index a module-scope const array, so the palette
// lives in a function-local var (addressable → runtime indexing is legal).
fn paletteColor(i: i32) -> vec3f {
    var palette = array<vec3f, 7>(
        vec3f(0.76, 0.33, 0.17),  // terra cotta
        vec3f(0.55, 0.62, 0.38),  // sage green
        vec3f(0.85, 0.68, 0.18),  // warm gold
        vec3f(0.92, 0.87, 0.72),  // cream
        vec3f(0.24, 0.48, 0.47),  // oxidized teal
        vec3f(0.35, 0.22, 0.14),  // walnut
        vec3f(0.72, 0.42, 0.12),  // burnt amber
    );
    return palette[clamp(i, 0, 6)];
}

// Low-frequency warp to break up uniform cell sizes (~15% variance)
fn warpUV(uv: vec2f) -> vec2f {
    let warpAmt = 0.15;
    let freq    = 3.0;
    let cell    = floor(uv * freq);
    var f       = fract(uv * freq);
    f = f * f * (3.0 - 2.0 * f);  // smoothstep

    let w00 = vec2f(hash21(cell)                 - 0.5, hash21( cell                 * 1.73 + 2.91) - 0.5);
    let w10 = vec2f(hash21(cell + vec2f(1.0, 0.0)) - 0.5, hash21((cell + vec2f(1.0, 0.0)) * 1.73 + 2.91) - 0.5);
    let w01 = vec2f(hash21(cell + vec2f(0.0, 1.0)) - 0.5, hash21((cell + vec2f(0.0, 1.0)) * 1.73 + 2.91) - 0.5);
    let w11 = vec2f(hash21(cell + vec2f(1.0, 1.0)) - 0.5, hash21((cell + vec2f(1.0, 1.0)) * 1.73 + 2.91) - 0.5);

    let warpVal = mix(mix(w00, w10, f.x), mix(w01, w11, f.x), f.y);
    return uv + warpVal * warpAmt / freq;
}

// 2D voronoi — returns vec2f(cellId, edgeDistance)
fn voronoi2D(p: vec2f, scale: f32) -> vec2f {
    let sp   = p * scale;
    let cell = floor(sp);
    var minDist    = 1.0;
    var secondDist = 1.0;
    var id = vec2f(0.0);

    for (var x = -1; x <= 1; x++) {
        for (var y = -1; y <= 1; y++) {
            let neighbor = cell + vec2f(f32(x), f32(y));
            let point = neighbor + vec2f(
                hash21(neighbor),
                hash21(neighbor * 1.37 + 7.31)
            );
            let d = length(sp - point);
            if (d < minDist) {
                secondDist = minDist;
                minDist = d;
                id = neighbor;
            } else if (d < secondDist) {
                secondDist = d;
            }
        }
    }
    return vec2f(hash21(id), secondDist - minDist);
}

// ── Skylight shading ────────────────────────────────────────────────────────

fn fragmentSkylight(in: VertexOutput) -> vec4f {
    // Radial UVs (0..1) derived from world position on the ceiling plane
    let uv = (in.worldPos.xz - SKYLIGHT_CENTER) / (SKYLIGHT_RADIUS * 2.0) + 0.5;

    let centered = uv - 0.5;
    let radial   = length(centered) * 2.0;

    // ── Two-layer voronoi for depth ─────────────────────────────
    let v1 = voronoi2D(warpUV(uv), 25.0);          // structural panes
    let v2 = voronoi2D(warpUV(uv + 3.17), 60.0);   // fine decorative inlay

    let cellId1 = v1.x;
    let edge1   = v1.y;
    let edge2   = v2.y;

    var glassColor  = paletteColor(i32(floor(cellId1 * 7.0)));
    let detailColor = paletteColor(i32(floor(v2.x * 7.0)));
    glassColor = mix(glassColor, detailColor, 0.15);

    // ── Per-pane glass translucency ─────────────────────────────
    var paneTranslucency = 0.27 + 0.24 * hash21(vec2f(cellId1, cellId1 * 2.71));
    var centerClear = 1.0 - smoothstep(0.0, 0.7, radial);
    centerClear = pow(centerClear, 1.5);
    paneTranslucency = clamp(paneTranslucency + centerClear * 0.18, 0.0, 0.68);

    // Sky tinted through each pane's glass color
    let tintedBeyond = BEYOND_COLOR * glassColor;
    var paneColor = mix(glassColor, tintedBeyond, paneTranslucency);
    paneColor += BEYOND_COLOR * paneTranslucency * 0.14;

    // ── Lead came ───────────────────────────────────────────────
    let lead1 = smoothstep(0.01, 0.04, edge1);
    let lead2 = smoothstep(0.005, 0.02, edge2);
    let lead  = min(lead1, mix(1.0, lead2, 0.4));  // secondary leads are subtler

    // ── Fresnel rim — skylight faces along ±Y instead of ±Z ─────
    // Note: on a perfectly flat disc this contributes ~0 everywhere
    // (same as the flat regions of the original). If you want the rim
    // effect without a dome bulge, swap in a view-vector fresnel:
    //   let V = normalize(u.cameraPos - in.worldPos);
    //   let fresnel = pow(1.0 - abs(dot(normalize(in.normal), V)), 2.5);
    let fresnel = pow(1.0 - abs(dot(normalize(in.normal), vec3f(0.0, 1.0, 0.0))), 2.5);

    // ── Compose ─────────────────────────────────────────────────
    var color = paneColor * lead * (0.48 + 0.22 * fresnel);

    // Back-illumination — brighter toward the clear center
    let backIllum = mix(0.42, 0.82, centerClear);
    color *= backIllum;

    // Sun-direction bias across the disc — this was the vertical
    // gradient on the wall portal; here it reads as one edge of the
    // skylight catching more daylight. Swap uv.y → uv.x (or remove)
    // to taste.
    let sunBias = mix(0.25, 1.5, smoothstep(0.50, 0.90, uv.y));
    color *= sunBias;

    // Lead came color — warm dark bronze
    color = mix(vec3f(0.05, 0.035, 0.02), color, lead);

    // Circular mask — gentle fade at the rim
    let circleMask = 1.0 - smoothstep(0.92, 1.0, radial);
    color *= circleMask;

    var alpha = GLASS_OPACITY * circleMask;
    alpha *= mix(0.88, 0.96, radial * 0.3);

    return vec4f(color, alpha);
}
