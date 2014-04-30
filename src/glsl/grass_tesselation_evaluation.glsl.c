layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch in float distance_tc, depth_tc;
patch in unsigned int instance_tc;

out vec3 position_te, tangent_te, bitangent_te, color_te;
out vec4 plane_te;
out float distance_te, height_te, parameter_te, depth_te, width_te;

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
    float phi, theta, cosphi, costheta, sinphi, sintheta, h_0, k;

    p = cluster_seed(apex_tc, left_tc, right_tc, stratum_tc, instance_tc);
    v = rand4();

    /* Calculate and set ouput values. */

    k = stiffness * distance_tc;
    k += (1 - k) * v.y;

    t = vec3(texture(deflections, vec2(gl_TessCoord.x, k)));
    
    phi = 2 * pi * v.z;
    theta = t.z;
    cosphi = cos(phi);
    sinphi = sin(phi);
    costheta = cos(theta);
    sintheta = sin(theta);
    
    d = (height.x + (height.y * distance_tc * v.x)) *
        vec3(cosphi, sinphi, 1) * t.xxy;

    plane_te = vec4(normal_tc, -dot(normal_tc, p));
    tangent_te = vec3(-cosphi * sintheta, -sinphi * sintheta, costheta);
    bitangent_te = vec3(-sinphi, cosphi, 0);
    position_te = p + d;
    
    color_te = color_tc;
    width_te = width.x + (width.y * distance_tc * v.w);
    distance_te = distance_tc;
    depth_te = depth_tc;
    parameter_te = gl_TessCoord.x;
    height_te = d.z / (height.x + height.y);
}
