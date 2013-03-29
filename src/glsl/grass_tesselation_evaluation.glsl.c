uniform sampler2D deflections;

void Grass_evaluation()
{
    vec3 p;
    vec2 uv;
    
    p = position_tc[0];
    uv = vec2(gl_TessCoord.x, 1);
 
    position_te = vec4(p + 0.2 * vec3(0, texture(deflections, uv).rg), 1);
}
