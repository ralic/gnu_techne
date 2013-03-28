void Grass_control()
{
    const float bias = 1, density = 16;
    
    vec3 p;
    float z, n;

    p = seed[0].position;
    z = min((modelview * vec4(p, 1)).z, -bias);
    n = bias * bias * density / z / z;
    
    gl_TessLevelOuter[0] = n;
    gl_TessLevelOuter[1] = 1.0;
    
    plant[gl_InvocationID].position = p;
}
