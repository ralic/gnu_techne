layout(isolines, equal_spacing) in;

patch in int index_tc;
flat out int index_te;

in vec3 position_tc[];
patch in vec3 color_tc;
patch in vec2 chance_tc;
patch in float distance_tc, depth_tc;
out vec4 position_te;
out vec3 color_te;
out float distance_te;
out mat2x3 plane_te;
flat out int category_te;
flat out float depth_te;

uniform sampler2D deflections;

void main()
{
    vec3 p, d, t;
    float phi, cosphi, sinphi;
    float theta, costheta, sintheta;
    int c;
    
    p = position_tc[0];

    phi = 2 * pi * chance_tc.x;
    cosphi = cos(phi);
    sinphi = sin(phi);
    c = 0;

    /* A stem segment. */
        
    t = vec3(texture(deflections, vec2(gl_TessCoord.x, 0.45 + 0.55 * chance_tc.y)));
    theta = t.b;
    costheta = cos(theta);
    sintheta = sin(theta);
        
    d = vec3(cosphi, sinphi, 1) * t.rrg;

    plane_te = mat2x3 (vec3(cosphi * costheta, costheta * sinphi, -sintheta),
                       vec3(-sinphi, cosphi,0));
    position_te = vec4(p + 0.05 * (1 + distance_tc + 0.5 * chance_tc.x) * d, 1);
    /* position_te = vec4(p + vec3(0, 0, 0.03 * gl_TessCoord.x), 1); */

    color_te = color_tc;
    distance_te = 1 + distance_tc + 0.5 * chance_tc.x;
    depth_te = depth_tc;
    category_te = c;
}
