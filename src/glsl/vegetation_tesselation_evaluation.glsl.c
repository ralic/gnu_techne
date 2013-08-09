layout(isolines, equal_spacing) in;

in vec3 position_tc[];
patch in float distance_tc, depth_tc;
patch in vec3 apex_tc, left_tc, right_tc, color_tc,  st_tc;
patch in int index_tc, instance_tc;

out vec3 position_te, color_te, tangent_te, bitangent_te;
out vec4 plane_te;
out float distance_te, height_te, depth_te, foo_te;
out int index_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
uniform sampler2D deflections;

vec2 hash(uvec2 U, unsigned int k);
vec4 rand4();
                                
void main()
{
    vec3 p, d, t, n;
    vec2 u;
    vec4 v;
    float phi, theta, cosphi, costheta, sinphi, sintheta, h_0, k;
    
    u = (hash(floatBitsToUint(position_tc[0].xy),
              floatBitsToUint(gl_TessCoord.y)) + st_tc.xy) / st_tc.z;
    
    float sqrtux;

    sqrtux = sqrt(u.x);
    
    p = (1 - sqrtux) * apex_tc +
        sqrtux * (1 - u.y) * left_tc +
        sqrtux * u.y * right_tc;
    
    v = rand4();
    
    /* Update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    /* Calculate and set ouput values. */

    h_0 = 0.35 + (0.65 * distance_tc * v.x);
    k = h_0 + (1 - h_0) * v.y;
    t = vec3(texture(deflections, vec2(gl_TessCoord.x, k)));
    phi = 2 * pi * v.z;
    theta = t.b;
    cosphi = cos(phi);
    sinphi = sin(phi);
    costheta = cos(theta);
    sintheta = sin(theta);
    
    d = vec3(cosphi, sinphi, 1) * t.rrg;
    n = normalize(cross(left_tc - apex_tc, right_tc - apex_tc));

    foo_te = t.g;
    plane_te = vec4(n, -dot(n, apex_tc));
    tangent_te = vec3(cosphi * costheta,
                      costheta * sinphi,
                      -sintheta);
    bitangent_te = vec3(-sinphi, cosphi,0);
    
    position_te = p + h_0 * 0.1 * d;
    /* position_te = vec4(p + vec3(0, 0, 0.03 * gl_TessCoord.x), 1); */
    
    color_te = color_tc;
    distance_te = 0.5 * distance_tc + 0.5 * v.w;
    depth_te = depth_tc;
    height_te = gl_TessCoord.x;
    index_te = index_tc;
}
