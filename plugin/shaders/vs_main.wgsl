// ── Projection helpers ────────────────────────────────────────────────────────
fn projectPerspective(pos: vec3f) -> vec4f {
    let depth = pos.z + 0.45;
    let inv_d = FOV_FACTOR / depth;
    return vec4f(pos.x * inv_d, pos.y * inv_d, pos.z * 0.5 + 0.5, 1.0);
}

fn projectFlat(pos: vec3f) -> vec4f {
    return vec4f(pos.x, pos.y, pos.z * 0.5 + 0.5, 1.0);
}

// ── Vertex entry point ────────────────────────────────────────────────────────
@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    var pos = in.position;

    switch u.materialId {
        case MAT_CAVE:             { out.clipPos = vsCave(pos);             }
        case MAT_SLIDER:           { out.clipPos = vsSlider(&pos, in.color); }
        case MAT_PLANE:            { out.clipPos = vsPlane(&pos);           }
        case MAT_FLOOR:            {out.clipPos  = vsFloor(&pos);            }
        case MAT_SKYLIGHT:         {out.clipPos  = vsSkylight(&pos);            }
        default:                   { out.clipPos = projectPerspective(pos); }
    }

    out.color    = in.color;
    out.worldPos = pos;
    out.normal   = in.normal;
    return out;
}
