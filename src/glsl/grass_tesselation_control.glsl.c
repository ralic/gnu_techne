void Grass_control()
{
    const vec3 offsets[3] = vec3[3](vec3(0, 0.005, 0), vec3(0, 0, 0.2), vec3(0, -0.005, 0));

    if(gl_InvocationID == 0) {
        color = seed[0].color;
        index = seed[0].index;
        
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
        gl_TessLevelInner[0] = 0.0;
        gl_TessLevelInner[1] = 0.0;
    }
    
    plant[gl_InvocationID].position = seed[0].position + offsets[gl_InvocationID];
}
