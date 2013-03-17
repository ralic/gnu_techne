void Grass_evaluation()
{
    vec3 p_0 = plant[0].position;
    vec3 p_1 = plant[1].position;
    vec3 p_2 = plant[2].position;
 
    vec3 c = gl_TessCoord.xyz;
 
    gl_Position = projection * modelview * vec4(p_0 * c.x + p_1 * c.y + p_2 * c.z, 1);
    shade = color;
}
