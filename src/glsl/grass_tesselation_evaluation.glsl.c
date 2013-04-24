in vec3 position_tc[];
patch in vec3 color_tc;
patch in vec2 chance_tc;
patch in float distance_tc;
out vec4 position_te;
out vec3 color_te;
out float distance_te;
out mat2x3 plane_te;
flat out int category_te;

uniform sampler2D deflections;

void Grass_evaluation()
{
    vec3 p, d, t;
    float phi, theta, cosphi, sinphi;
    int c;
    
    p = position_tc[0];
    phi = 2 * pi * chance_tc.y;
    cosphi = cos(phi);
    sinphi = sin(phi);
    c = int(round(gl_TessCoord.y * gl_TessLevelOuter[0]));

    if (c == 0) {
        float theta, costheta, sintheta;

        /* A stem segment. */
        
        t = texture(deflections, vec2(gl_TessCoord.x, chance_tc.x));
        theta = t.b;
        costheta = cos(theta);
        sintheta = sin(theta);
        
        d = vec3(cosphi, sinphi, 1) * t.rrg;

        plane_te = mat2x3 (vec3(cosphi * costheta, costheta * sinphi, -sintheta),
                           vec3(-sinphi, cosphi,0));
        position_te = vec4(p + d, 1);
    } else {
        float psi, cospsi, sinpsi;
        vec3 d_1, t_1;

        /* A leaf segment. */
        
        t = vec3(texture(deflections, vec2(gl_TessCoord.y / gl_TessLevelOuter[0], chance_tc.x)));

        psi = 0.5 * pi + t.b;
        cospsi = cos(psi);
        sinpsi = sin(psi);
        d = vec3(cosphi, sinphi, 1) * t.rrg;
        
        t_1 = vec3(texture(deflections, vec2(gl_TessCoord.x, 1.0)));
        d_1 = cospsi * vec3(t_1.x, 0, t_1.y) + sinpsi * vec3(t_1.y, 0, -t_1.x);

        plane_te = mat2x3(vec3(-sinphi, cosphi, 0), vec3(0));
        position_te = vec4(p + d + 0.05 * d_1, 1);
    }
    
    color_te = color_tc;
    distance_te = distance_tc;
    category_te = c;
}
