layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc;
patch in vec4 color_tc;
patch in float distance_tc, depth_tc;
patch in unsigned int instance_tc;

out vec3 position_te, tangent_te, bitangent_te;
out vec4 plane_te, color_te;
out float distance_te, height_te, depth_te, width_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif

vec4 rand4();
void srand(uvec2 seed);
                                
uniform sampler2D deflections;
      
uniform grass_evaluation {
    vec2 height, width;
    float stiffness;
};
                                
void main()
{
    vec3 p, d, t, n;
    vec4 v;
    vec2 u;
    float sqrtux, phi, theta, cosphi, costheta, sinphi, sintheta, h_0, k;

    u = hash(floatBitsToUint((apex_tc.xy + left_tc.xy + right_tc.xy) / 3),
             floatBitsToUint((gl_TessCoord.y + 1) * (instance_tc + 1)));
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
    
    v = rand4();
    
    /* update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    /* Calculate and set ouput values. */

    k = stiffness * distance_tc;
    k += (1 - k) * v.y;

    t = vec3(texture(deflections, vec2(gl_TessCoord.x, k)));
    
    phi = 2 * pi * v.z;
    theta = t.b;
    cosphi = cos(phi);
    sinphi = sin(phi);
    costheta = cos(theta);
    sintheta = sin(theta);
    
    d = vec3(cosphi, sinphi, 1) * t.rrg;

    plane_te = vec4(normal_tc, -dot(normal_tc, p));
    tangent_te = vec3(-cosphi * sintheta, sintheta * sinphi, costheta);
    bitangent_te = vec3(-sinphi, cosphi, 0);
    position_te = p + (height.x + (height.y * distance_tc * v.x)) * d;
    
    color_te = color_tc;
    width_te = width.x + (width.y * distance_tc * v.w);
    distance_te = distance_tc;
    depth_te = depth_tc;
    height_te = gl_TessCoord.x;
}
