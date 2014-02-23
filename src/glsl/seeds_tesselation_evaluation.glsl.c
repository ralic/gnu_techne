layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch in unsigned int instance_tc;

flat out vec3 color_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
uniform seeds_evaluation {
    float height;
};

void main()
{
    vec3 p, n;
    vec2 u;
    float sqrtux;
    
    /* Update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    u = hash(floatBitsToUint(3 * apex_tc.xy + 5 * left_tc.xy + 7 * right_tc.xy),
             floatBitsToUint(gl_TessCoord.y) + instance_tc);
    u = (u + stratum_tc.xy) / stratum_tc.z;

    if (false) {
        sqrtux = sqrt(u.x);

        p = ((1 - sqrtux) * apex_tc +
             sqrtux * (1 - u.y) * left_tc +
             sqrtux * u.y * right_tc);
    } else {
        u = mix(u, 1 - u.yx, floor(u.x + u.y));
        p = apex_tc + (left_tc - apex_tc) * u.x + (right_tc - apex_tc) * u.y;
    }
    
    gl_Position = transform * vec4(p + height * gl_TessCoord.x * normal_tc, 1);
    color_te = color_tc;
}
