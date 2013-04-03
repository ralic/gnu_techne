in vec3 position_tc[];
patch in vec3 color_tc;
out vec4 position_te;
out vec3 color_te;

uniform sampler2D deflections;

void Grass_evaluation()
{
    vec3 p;
    vec2 uv;
    
    p = position_tc[0];
    uv = vec2(gl_TessCoord.x, 0.06);
    
    color_te = color_tc; 
    position_te = vec4(p + 0.2 * vec3(0, texture(deflections, uv).rg), 1);
}
