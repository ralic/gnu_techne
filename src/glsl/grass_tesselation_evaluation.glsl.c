void Grass_evaluation()
{
    const vec3 p_0 = vec3(0, 0.005, 0);
    const vec3 p_1 = vec3(0, 0, 0.2);
    const vec3 p_2 = vec3(0, -0.005, 0);
 
    vec3 c, p;
    
    p = plant[0].position;
    c = gl_TessCoord.xyz;
 
    gl_Position = vec4(p + p_1 * c.x /* + p_1 * c.y + p_2 * c.z */, 1);
}
