//===============================================
//ADSR ramp — port of TDS-01's
//  WebUI/AmpEnvelope/adsrVertexShader.glsl   (the whole shape)
//  WebUI/AmpEnvelope/adsrFragmentShader.glsl (shading)
//
//The incoming geometry is a bare UV grid with zero positions; every vertex
//position is computed here from a 4-segment catmull-rom through the ADSR
//control points, with a fixed Z binormal so the tube cannot twist.
//
//UV arrives in in.color.xy. in.color.z flags an end cap (0 = ramp, 1 = cap),
//which skips the displacement.
//
//u.adsrShape = (attack, decay, sustain, release)  — normalised, summing to 1
//u.adsrDims  = (width, height, tubeRadius, sustainLevel)
//===============================================

const ADSR_TENSION: f32 = 0.4;
 const ADSR_ORIGIN:  vec3f = vec3f(0.89, 0.315, 1.81);
  const ADSR_SCALE:   f32 = 0.1;    // about 0.35 wide, 0.15 tall

//===============================================
//Control points — ghost points at each end give the
//curve a horizontal approach and exit.
//===============================================
fn adsrControlPoint(index: i32, w: f32, h: f32, yS: f32,
                    x0: f32, xA: f32, xD: f32, xS: f32, xR: f32) -> vec3f {
    if (index <= 0) {
        return vec3f(x0 - w * 0.1, 0.0, 0.0);
    } else if (index == 1) {
        return vec3f(x0, 0.0, 0.0);
    } else if (index == 2) {
        return vec3f(xA, h, 0.0);
    } else if (index == 3) {
        return vec3f(xD, yS, 0.0);
    } else if (index == 4) {
        return vec3f(xS, yS, 0.0);
    } else if (index == 5) {
        return vec3f(xR, 0.0, 0.0);
    }
    return vec3f(xR + w * 0.1, 0.0, 0.0);
}

fn adsrCatmullRom(p0: vec3f, p1: vec3f, p2: vec3f, p3: vec3f,
                  t: f32, tension: f32) -> vec3f {
    let t2 = t * t;
    let t3 = t2 * t;
    let alpha = tension;

    let m1 = (p2 - p0) * (1.0 - alpha) * 0.5;
    let m2 = (p3 - p1) * (1.0 - alpha) * 0.5;

    let a =  2.0 * t3 - 3.0 * t2 + 1.0;
    let b =        t3 - 2.0 * t2 + t;
    let c = -2.0 * t3 + 3.0 * t2;
    let d =        t3 -       t2;

    return a * p1 + b * m1 + c * p2 + d * m2;
}

fn adsrCatmullRomDerivative(p0: vec3f, p1: vec3f, p2: vec3f, p3: vec3f,
                            t: f32, tension: f32) -> vec3f {
    let t2 = t * t;
    let alpha = tension;

    let m1 = (p2 - p0) * (1.0 - alpha) * 0.5;
    let m2 = (p3 - p1) * (1.0 - alpha) * 0.5;

    let a =  6.0 * t2 - 6.0 * t;
    let b =  3.0 * t2 - 4.0 * t + 1.0;
    let c = -6.0 * t2 + 6.0 * t;
    let d =  3.0 * t2 - 2.0 * t;

    return a * p1 + b * m1 + c * p2 + d * m2;
}

//===============================================
//Vertex
//===============================================
fn vertexAdsr(pos: ptr<function, vec3f>, uvIn: vec3f,
              nrm: ptr<function, vec3f>) -> vec4f {
    // End caps come in as real geometry; only place them.
    if (uvIn.z > 0.5) {
        let capWorld = *pos * ADSR_SCALE + ADSR_ORIGIN;
        *pos = capWorld;
        return projectPerspective(capWorld);
    }

    let t     = uvIn.x;
    let angle = uvIn.y * 2.0 * 3.14159265359;

    let w  = u.adsrDims.x;
    let h  = u.adsrDims.y;
    let yS = u.adsrDims.w * h;

    let x0 = -w * 0.5;
    let xA = x0 + w * u.adsrShape.x;
    let xD = xA + w * u.adsrShape.y;
    let xS = xD + w * u.adsrShape.z;
    let xR = x0 + w;

    let numSegments = 4.0;
    let scaledT = t * numSegments;
    var segmentIndex = floor(scaledT);
    var localT = fract(scaledT);

    if (t >= 1.0) {
        segmentIndex = 3.0;
        localT = 1.0;
    }

    let seg = i32(segmentIndex);

    let p0 = adsrControlPoint(seg + 0, w, h, yS, x0, xA, xD, xS, xR);
    let p1 = adsrControlPoint(seg + 1, w, h, yS, x0, xA, xD, xS, xR);
    let p2 = adsrControlPoint(seg + 2, w, h, yS, x0, xA, xD, xS, xR);
    let p3 = adsrControlPoint(seg + 3, w, h, yS, x0, xA, xD, xS, xR);

    let curvePos = adsrCatmullRom(p0, p1, p2, p3, localT, ADSR_TENSION);
    var tangent  = adsrCatmullRomDerivative(p0, p1, p2, p3, localT, ADSR_TENSION);

    let tangentLen = length(tangent);
    if (tangentLen < 0.001) {
        tangent = vec3f(1.0, 0.0, 0.0);
    } else {
        tangent = tangent / tangentLen;
    }

    // Fixed reference frame — the curve is planar (Z = 0), so this prevents
    // any twist along the tube.
    let binormal = vec3f(0.0, 0.0, 1.0);
    var normal   = normalize(cross(binormal, tangent));

    if (length(normal) < 0.001) {
        normal = vec3f(0.0, 1.0, 0.0);
    }

    let offset   = (cos(angle) * normal + sin(angle) * binormal) * u.adsrDims.z;
    let localPos = curvePos + offset;

    // vNormal in the JS vertex shader
    *nrm = normalize(cos(angle) * normal + sin(angle) * binormal);

    // group scale 0.5, then AE_LOCATIONS.AMP_ENV_Z
    let world = localPos * ADSR_SCALE + ADSR_ORIGIN;

    *pos = world;
    return projectPerspective(world);
}

//===============================================
//Fragment — adsrFragmentShader.glsl
//===============================================
fn fragmentAdsr(in: VertexOutput) -> vec4f {
    // createAdsr() is handed oscillatorOneColor 0xff8800 with a terracotta
    // emissive at emissiveIntensity 0.5 — the same material config as
    // oscModuleMaterial, so the ramp and oscillator one are meant to be the
    // same colour. The 0.5 has to be applied: at full strength the terracotta
    // outweighs the albedo and the ramp reads pink rather than orange.
    let uColor    = vec3f(1.000, 0.533, 0.000);
    let uEmissive = vec3f(0.776, 0.463, 0.314) * 0.5;

    // The ramp writes its own tube normal; caps carry real geometry normals.
    var nrm = normalize(in.normal);

    let lightDir1 = normalize(vec3f( 0.5,  1.0, 0.3));
    let lightDir2 = normalize(vec3f(-0.3, -0.5, 0.8));

    let diff1 = max(dot(nrm, lightDir1), 0.0);
    let diff2 = max(dot(nrm, lightDir2), 0.0) * 0.3;
    //light experiment=====================================
    var light = vec3f(0.0);
//
    let viewDirection = normalize(u.cameraPosition - in.worldPos.xyz);
//
    light += ambientLight(in.worldPos.xyz,
                                nrm,
                                vec3f(1.0, 0.0, 0.0),
                                0.2);
//
    let modelNormal = u.modelMatrix * vec4(nrm, 0.0);
    light += directionalLight(in.worldPos.xyz,
                                  modelNormal.xyz,
                                  vec3f(0.1, 0.1, 0.1),
                                   0.5,
                                  vec3f(0.0, 1.0, 0.3),
                                  viewDirection
                                  );
    //Done==================================================



    let lighting = diff1 * 0.01 + diff2;
    let color = uColor * light + uEmissive;

    return vec4f(color, 1.0);
}
