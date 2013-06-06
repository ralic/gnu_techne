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
    const float bias = 1, density = 16;
    
    vec3 p;
    float z, n;

    p = position[0];
    z = max(-(modelview * vec4(p, 1)).z, bias);
    n = bias * density / z;
    
    gl_TessLevelOuter[0] = 1.0;
    gl_TessLevelOuter[1] = n;
    
    color_tc = color[0];    
    position_tc[gl_InvocationID] = p;
    distance_tc = distance[0];
    depth_tc = z;
    chance_tc = chance[0];
}
