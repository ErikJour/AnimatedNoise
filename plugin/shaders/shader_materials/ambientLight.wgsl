//=======================================================
//Ambient Lighting
//=========================================================
fn ambientLight(worldPos: vec3f, normal: vec3f, lightColor: vec3f, lightIntensity: f32) -> vec3f {

    return lightColor * lightIntensity;

}