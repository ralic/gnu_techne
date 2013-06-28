layout(vertices = 1) out;

flat in int index[];
patch out int index_tc;

in vec3 position[], color[];
flat in vec2 chance[];
in float distance[];
out vec3 position_tc[];
patch out vec3 color_tc;
patch out vec2 chance_tc;
patch out float distance_tc, depth_tc;

void main() {
    const float bias = 1;
    float densities[4] = {8, 0, 0, 0};
    
    vec3 p;
    vec4 p_e;
    float z, n;
    int i;
    
    i = index[0];

    if (i < 0) {
        gl_TessLevelOuter[0] = 0;
        gl_TessLevelOuter[1] = 0;

        return;
    }

    p = position[0];
    p_e = modelview * vec4(p, 1);
    z = max(-p_e.z, bias);
    n = bias * densities[i] / z;
    
    gl_TessLevelOuter[0] = 1.0;
    gl_TessLevelOuter[1] = n;
    
    color_tc = color[0];    
    position_tc[gl_InvocationID] = p;
    index_tc = i;
    distance_tc = distance[0];
    depth_tc = z;
    chance_tc = chance[0];
}
