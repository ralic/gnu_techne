layout(isolines, equal_spacing) in;

in vec3 position_tc[];
patch in vec3 color_tc;
patch in float distance_tc, depth_tc;
patch in vec3 apex_tc, left_tc, right_tc;
patch in int index_tc, instance_tc;

out vec3 position_te, color_te;
out float distance_te, height_te;
out mat2x3 plane_te;
flat out int index_te;
flat out float depth_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
uniform sampler2D deflections;
uniform vec3 clustering;

vec2 hash(uvec2 U, unsigned int k);
vec2 rand();
                                
void main()
{
    vec3 p, d, t;
    vec2 u, v;
    float phi, theta, h_0;
    
    u = hash(floatBitsToUint(position_tc[0].xy),
             floatBitsToUint(gl_TessCoord.y));
    
    if (instance_tc > clustering[2]) {
        vec3 s, t;
        
        u = 2 * (u - 0.5);
        s = clustering[1] * normalize(left_tc - apex_tc);
        t = clustering[1] * normalize(right_tc - apex_tc);
        p = position_tc[0] + u.x * s + u.y * t;
    } else {
        float sqrtux;

        sqrtux = sqrt(u.x);
    
        p = (1 - sqrtux) * apex_tc +
            sqrtux * (1 - u.y) * left_tc +
            sqrtux * u.y * right_tc;
    }
    
    u = rand();
    v = rand();
    
    /* Update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    /* Calculate and set ouput values. */

    h_0 = 0.15 + (0.85 * distance_tc * v.x);
    t = vec3(texture(deflections, vec2(gl_TessCoord.x, h_0 + (1 - h_0) * u.y)));
    phi = 2 * pi * u.x;
    theta = t.b;
    d = vec3(cos(phi), sin(phi), 1) * t.rrg;

    plane_te = mat2x3 (vec3(cos(phi) * cos(theta),
                            cos(theta) * sin(phi),
                            -sin(theta)),
                       vec3(-sin(phi), cos(phi),0));
    position_te = p + h_0 * 0.1 * d;
    /* position_te = vec4(p + vec3(0, 0, 0.03 * gl_TessCoord.x), 1); */
    
    color_te = color_tc;
    distance_te = 0.75 * distance_tc + 0.25 * v.y;
    depth_te = depth_tc;
    height_te = gl_TessCoord.x;
    index_te = index_tc;
}
