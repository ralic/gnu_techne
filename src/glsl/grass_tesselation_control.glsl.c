layout(vertices = 1) out;

in vec3 position_v[], normal_v[];
in vec4 color_v[];
in float distance_v[];

out vec3 position_tc[];
patch out vec3 normal_tc;
patch out vec4 color_tc;
patch out int instance_tc;
patch out float distance_tc, depth_tc;

void main() {
    const float bias = 1;
    const float density = 8;
    
    vec3 p;
    vec4 p_e;
    float z, n;
    int i;

    p = position_v[0];
    p_e = modelview * vec4(p, 1);
    z = max(-p_e.z, bias);
    n = bias * density / z / z / z;
    
    gl_TessLevelOuter[0] = 1;
    gl_TessLevelOuter[1] = n;
    
    color_tc = color_v[0];
    normal_tc = normal_v[0];
    position_tc[gl_InvocationID] = p;
    distance_tc = distance_v[0];
    instance_tc = gl_PrimitiveID;
    depth_tc = z;
}
