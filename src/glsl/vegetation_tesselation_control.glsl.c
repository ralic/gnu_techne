layout(vertices = 1) out;

in vec3 position[], color[];
in float distance[];
flat in int index[], instance[];
flat in vec3 apex_v[], left_v[], right_v[];

out vec3 position_tc[];
patch out vec3 apex_tc, left_tc, right_tc;
patch out int index_tc, instance_tc;
patch out vec3 color_tc;
patch out float distance_tc, depth_tc;

uniform vec3 clustering;

void main() {
    const float bias = 1;
    const float densities[4] = {8, 0, 0, 0};
    
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
    
    gl_TessLevelOuter[0] = clustering[0];
    gl_TessLevelOuter[1] = n;
    
    color_tc = color[0];    
    position_tc[gl_InvocationID] = p;
    index_tc = i;
    instance_tc = instance[0];
    distance_tc = distance[0];
    depth_tc = z;

    apex_tc = apex_v[0]; left_tc = left_v[0]; right_tc = right_v[0];
}