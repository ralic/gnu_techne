in vec3 position_tc[];
patch in vec3 color_tc;
patch in vec2 chance_tc;
patch in float distance_tc;
out vec4 position_te;
out vec3 color_te;
out float distance_te;
out int category_te;

uniform sampler2D deflections;

void Grass_evaluation()
{
    vec3 p, r, d;
    int c;
    
    p = position_tc[0];
    /* r = vec3(cos(2 * pi * chance_tc.y), sin(2 * pi * chance_tc.y), 1); */
    r = vec3(1, 0, 1);
    c = int(round(gl_TessCoord.y * gl_TessLevelOuter[0]));

    if (c == 0) {
        /* A stem segment. */
        
        d = texture(deflections, vec2(gl_TessCoord.x, chance_tc.x)).rrg;
    
        position_te = vec4(p + r * d, 1);
    } else {
        float s, c;
        vec3 d_1, r_1;

        d = texture(deflections, vec2(gl_TessCoord.y / gl_TessLevelOuter[0], chance_tc.x)).rgb;
        
        s = sin(0.5 * pi + d.b);
        c = cos(0.5 * pi + d.b);
        d_1 = vec3(texture(deflections, vec2(gl_TessCoord.x, 1.0)));
        r_1 = c * vec3(d_1.x, 0, d_1.y) + s * vec3(d_1.y, 0, -d_1.x);
    
        position_te = vec4(p + r * d.rrg + 0.05 * r_1, 1);
    }
    
    color_te = color_tc;
    distance_te = distance_tc;
    category_te = c;
}
