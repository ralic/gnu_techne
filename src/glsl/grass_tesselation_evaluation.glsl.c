in vec3 position_tc[];
patch in vec3 color_tc;
patch in vec2 chance_tc;
patch in float distance_tc;
out vec4 position_te;
out vec3 color_te;
out float distance_te;

uniform sampler2D deflections;

void Grass_evaluation()
{
    vec3 p, r, d;
    vec2 uv;
    
    p = position_tc[0];
    uv = vec2(gl_TessCoord.x, chance_tc.x);
    r = vec3(cos(2 * pi * chance_tc.y), sin(2 * pi * chance_tc.y), 1);
    d = texture(deflections, uv).rrg;
    
    color_te = color_tc;
    position_te = vec4(p + 0.2 * (0.5 + 0.5 * chance_tc.x) * distance_tc * r * d, 1);
    distance_te = distance_tc;
}
