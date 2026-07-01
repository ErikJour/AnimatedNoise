fn centerLight(worldPos: vec3f) -> vec3f {
    let hy: f32 = 0.25;
    let c0 = vec2f(  0.000,  0.0000 );
    return vec3f(c0.x, hy, c0.y);
}

fn ceilingLights(worldPos: vec3f) -> vec3f {
    let lightHeight: f32 = 1.75;
    let lightOne   = vec2f(  1.900,  0.0000);
    let lightTwo   = vec2f(  0.950,  1.6454);
    let LightThree = vec2f( -0.950,  1.6454);
    let lightFour  = vec2f( -1.900,  0.0000);
    let lightFive  = vec2f( -0.950, -1.6454);
    let lightSix   = vec2f(  0.950, -1.6454);

    var currentLight = lightOne;

    var bd = distance(worldPos.xz, lightOne);
    let d1 = distance(worldPos.xz, lightOne);   if (d1 < bd) { bd = d1; currentLight = lightOne; }
    let d2 = distance(worldPos.xz, lightTwo);   if (d2 < bd) { bd = d2; currentLight = lightTwo; }
    let d3 = distance(worldPos.xz, LightThree); if (d3 < bd) { bd = d3; currentLight = LightThree; }
    let d4 = distance(worldPos.xz, lightFour);  if (d4 < bd) { bd = d4; currentLight = lightFour; }
    let d5 = distance(worldPos.xz, lightFive);  if (d5 < bd) { bd = d5; currentLight = lightFive; }
    let d6 = distance(worldPos.xz, lightSix);   if (d6 < bd) {           currentLight = lightSix; }

    return vec3f(currentLight.x, lightHeight, currentLight.y);
}

fn roomPointLight(worldPos: vec3f, normal: vec3f) -> vec3f {
    let wave       = sin(u.time);
    //Center Light
    let lightPos   = centerLight(worldPos);
    let toLight    = lightPos - worldPos;
    let dist       = length(toLight);
    let lightDir   = toLight / dist;
    let atten      = 1.0 / (1.0 + 12.0 * dist * dist * dist);
    let diffuse    = max(dot(normal, lightDir), 0.0) * atten;
    //Top Lights
    let lightPosTwo   = ceilingLights(worldPos);
    let toLightTwo    = lightPosTwo - worldPos;
    let distTwo       = length(toLightTwo);
    let lightDirTwo   = toLightTwo / distTwo;
    let attenTwo      = 1.0 / (1.0 + 12.0 * distTwo * distTwo * distTwo);
    let diffuseTwo    = max(dot(normal, lightDirTwo), 0.0) * attenTwo;
    //Mix
    let lampColor  = vec3f(1.0, 0.92, 0.80);
    let ambient    = vec3f(0.035, 0.18, 0.3);
    return ambient + (diffuse * lampColor) + (diffuseTwo * lampColor);
}

fn computeNormal(worldPos: vec3f) -> vec3f {
    let dp_dx  = dpdx(worldPos);
    let dp_dy  = dpdy(worldPos);
    var normal = normalize(cross(dp_dx, dp_dy));
    if dot(normal, u.lightPos - worldPos) < 0.0 { normal = -normal; }
    return normal;
}

fn pointLight(worldPos: vec3f, normal: vec3f) -> vec3f {
    let toLight     = u.lightPos  - worldPos;
    let dist        = length(toLight);
    let lightDir    = toLight / dist;
    let attenuation = 1.0 / (1.0 + 12.0 * dist * dist);
    let diffuse     = max(dot(normal, lightDir), 0.0) * attenuation;
    let lampColor   = vec3f(1.0, 0.92, 0.80);
    let ambient     = vec3f(0.5, 0.8, 0.3);
    return ambient + diffuse * lampColor;
}

