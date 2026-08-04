//============================================
//Vertex
//============================================
fn vertexSkylight(pos: ptr<function, vec3f>) -> vec4f {
    var worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    worldPos     *= 0.5;
    worldPos.y   -= 0.9;
    *pos         = worldPos.xyz;
    return projectPerspective(worldPos.xyz);
}
//============================================
//Fragment
//============================================
fn fragmentSkylight(in: VertexOutput) -> vec4f {
    let uv            = (in.worldPos.xz);
    let normal        = normalize(in.normal);
    var light         = vec3f(0.0);
    let viewDirection = normalize(u.cameraPosition - in.worldPos.xyz);

    light += ambientLight(in.worldPos.xyz,
                          normal,
                          vec3f(1.0, 0.0, 0.0),
                          0.4);

    let modelNormal = u.modelMatrix * vec4(normal, 0.0);
    light += directionalLight(in.worldPos.xyz,
                                   modelNormal.xyz,
                                   vec3f(0.1, 0.1, 0.1),
                                    0.1,
                                   vec3f(0.0, -0.33, 0.0),
                                   viewDirection
                                   );

    let wave       = sin(u.time * 0.25);
    var red        = wave * 0.7;
    let color      = vec3f(red, 0.7, 0.8);
    return vec4f((color * light) * 0.36, 0.1);
}
