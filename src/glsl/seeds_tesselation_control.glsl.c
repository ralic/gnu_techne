layout(vertices = 1) out;

in vec3 position_v[], normal_v[];
in vec4 color_v[];

out vec3 position_tc[];
patch out vec3 normal_tc;
patch out vec4 color_tc;

void main() {
    gl_TessLevelOuter[0] = 1;
    gl_TessLevelOuter[1] = 1;
    
    color_tc = color_v[0];
    normal_tc = normal_v[0];
    position_tc[gl_InvocationID] = position_v[0];
}
