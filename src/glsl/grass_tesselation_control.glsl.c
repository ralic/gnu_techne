layout(vertices = 1) out;

in vec3 position[];
in vec3 color[];
out vec3 position_tc[];
patch out vec3 color_tc;

void Grass_control()
{
    const float bias = 1, density = 16;
    
    vec3 p;
    float z, n;

    p = position[0];
    z = min((modelview * vec4(p, 1)).z, -bias);
    n = bias * bias * density / z / z;
    
    gl_TessLevelOuter[0] = n;
    gl_TessLevelOuter[1] = 1.0;
    
    color_tc = color[0];    
    position_tc[gl_InvocationID] = p;
}
