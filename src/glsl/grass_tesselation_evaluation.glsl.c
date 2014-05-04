layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch in float score_tc, depth_tc;
patch in unsigned int instance_tc;

out vec3 position_te, normal_te, side_te, color_te;
out vec4 plane_te;
out float score_te, height_te, parameter_te, depth_te, width_te;

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

    /* Determine the position of the root of the currently tessellated
     * plant. */

    p = cluster_seed(apex_tc, left_tc, right_tc, stratum_tc, instance_tc);
    v = rand4();

    /* Determine the plant's stiffness. */

    k = stiffness * score_tc;
    k += (1 - k) * v.y;

    /* Sample the deflection texture.  We pass in the current distance
     * along the grass blade and the blade stiffness and get a triplet
     * with the lateral deflection, height above ground and
     * inclination angle. */

    t = vec3(texture(deflections, vec2(gl_TessCoord.x, k)));

    /* Orient the blade randomly and calculate the position of the
     * current segment vertex. */

    phi = 2 * pi * v.z;
    theta = t.z;
    cosphi = cos(phi);
    sinphi = sin(phi);
    costheta = cos(theta);
    sintheta = sin(theta);

    d = (height.x + (height.y * score_tc * v.x)) *
        vec3(cosphi, sinphi, 1) * t.xxy;

    position_te = p + d;

    /* Caclulate the plane coefficients of the seeded triangle (needed
     * for projected shadows later on) as well as two more verctors
     * pointing along the side of the blade and normal to it. */

    plane_te = vec4(normal_tc, -dot(normal_tc, p));
    normal_te = vec3(-cosphi * sintheta, -sinphi * sintheta, costheta);
    side_te = vec3(-sinphi, cosphi, 0);

    color_te = color_tc;
    width_te = width.x + (width.y * score_tc * v.w);
    score_te = score_tc;
    depth_te = depth_tc;
    parameter_te = gl_TessCoord.x;
    height_te = d.z / (height.x + height.y);
}
