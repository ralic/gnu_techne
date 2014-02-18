layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc;
patch in vec4 color_tc;
patch in uvec2 chance_tc;

out vec4 color_te;
                                
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

    u = hash(chance_tc, floatBitsToUint(gl_TessCoord.y));
    u = (u + stratum_tc.xy) / stratum_tc.z;

    sqrtux = sqrt(u.x);

    p = ((1 - sqrtux) * apex_tc +
         sqrtux * (1 - u.y) * left_tc +
         sqrtux * u.y * right_tc);
    n = normalize(cross(left_tc - apex_tc, right_tc - apex_tc));

    gl_Position = transform * vec4(p + height * gl_TessCoord.x * n, 1);
    color_te = color_tc;
}
