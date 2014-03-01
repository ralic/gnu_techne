layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch in unsigned int instance_tc;

out vec3 normal_te, position_te;
out vec4 plane_te, chance_te;
out vec3 color_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
uniform gravel_evaluation {
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

    u = mix(u, 1 - u.yx, floor(u.x + u.y));
    p = apex_tc + (left_tc - apex_tc) * u.x + (right_tc - apex_tc) * u.y;

    {
        vec3 u, v, rho;
        float phi;
        
        phi = 2 * pi * gl_TessCoord.x;
        
        u = normalize(vec3(-normal_tc.y, normal_tc.x, 0));
        v = normal_tc;

        rho = vec3(cos(phi), sin(phi), 0);

        plane_te = vec4(normal_tc, -dot(normal_tc, p));
        normal_te = rho;
        position_te = p;
    }
    
    color_te = color_tc;
    chance_te = rand4();
}
