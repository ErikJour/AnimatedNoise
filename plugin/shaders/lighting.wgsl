// ── Lighting helpers ──────────────────────────────────────────────────────────
fn computeNormal(worldPos: vec3f) -> vec3f {
    let dp_dx  = dpdx(worldPos);
    let dp_dy  = dpdy(worldPos);
    var normal = normalize(cross(dp_dx, dp_dy));
    if dot(normal, u.lightPos - worldPos) < 0.0 { normal = -normal; }
    return normal;
}

fn pointLight(worldPos: vec3f, normal: vec3f) -> vec3f {
    let wave = sin(u.frequency + u.time) * u.amplitude;

//    var offsetA     = vec3f(wave, wave, 0.1);
    let toLight     = u.lightPos  - worldPos;
    let dist        = length(toLight);
    let lightDir    = toLight / dist;
    let attenuation = 1.0 / (1.0 + 4.0 * dist * dist);
    let diffuse     = max(dot(normal, lightDir), 0.0) * attenuation;
    let lampColor   = vec3f(1.0, 0.92, 0.80);
    let ambient    = vec3f(0.18, 0.25, 0.02);
    return ambient + diffuse * lampColor;
}
