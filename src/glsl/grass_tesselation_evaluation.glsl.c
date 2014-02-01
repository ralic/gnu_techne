layout(isolines, equal_spacing) in;

in vec3 position_tc[];
patch in float distance_tc, depth_tc;
patch in vec4 color_tc;
patch in vec3 normal_tc;
patch in int instance_tc;

out vec3 position_te, tangent_te, bitangent_te;
out vec4 plane_te, color_te;
out float distance_te, height_te, depth_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif

vec2 rand2();
void srand(uvec2 seed);
                                
uniform sampler2D deflections;
              
uniform grass_debug{
    int debug;
};
                                
void main()
{
    vec3 p, d, t, n;
    vec4 v;
    float phi, theta, cosphi, costheta, sinphi, sintheta, h_0, k;

    p = position_tc[0];

    hash(floatBitsToUint(p.xy), floatBitsToUint(color_tc.r));
    v = rand4();
    
    /* update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    /* Calculate and set ouput values. */

    h_0 = 0.35 + (0.65 * distance_tc * v.x);
    k = 0.3 * distance_tc + (1 - 0.3 * distance_tc) * v.y;
    t = vec3(texture(deflections, vec2(gl_TessCoord.x, k)));
    phi = 2 * pi * v.z;
    theta = t.b;
    cosphi = cos(phi);
    sinphi = sin(phi);
    costheta = cos(theta);
    sintheta = sin(theta);
    
    d = vec3(cosphi, sinphi, 1) * t.rrg;
    n = normal_tc;

    plane_te = vec4(n, -dot(n, p));
    tangent_te = vec3(cosphi * costheta, costheta * sinphi, -sintheta);
    bitangent_te = vec3(-sinphi, cosphi, 0);
    position_te = p + h_0 * 0.1 * d;

    /* tangent_te = vec3(0, 0, 1); */
    /* bitangent_te = vec3(0, 1, 0); */
    /* position_te = p + vec3(0, 0, 0.03 * gl_TessCoord.x); */
    
    color_te = color_tc;
    distance_te = /* 0.5 *  */distance_tc /* + 0.5 * v.w */;
    depth_te = depth_tc;
    height_te = gl_TessCoord.x;
}
