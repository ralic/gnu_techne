layout(vertices = 1) out;

in vec3 apex_v[], left_v[], right_v[], stratum_v[];
in vec4 color_v[];
in float distance_v[];
in float clustering_v[];
in uvec2 chance_v[];

patch out vec3 apex_tc, left_tc, right_tc, stratum_tc;
patch out vec4 color_tc;
patch out float distance_tc, depth_tc;
patch out uvec2 chance_tc;

uniform grass_control {
    float detail;
};

void main() {
    const float bias = 1;
    
    vec4 p_e;
    float z, n;
    int i;

    p_e = modelview * vec4((apex_v[0] + left_v[0] + right_v[0]) / 3, 1);
    z = max(-p_e.z, bias);
    n = bias * detail / z / z;
    
    gl_TessLevelOuter[0] = clustering_v[0];
    gl_TessLevelOuter[1] = n;

    apex_tc = apex_v[0];
    left_tc = left_v[0];
    right_tc = right_v[0];
    stratum_tc = stratum_v[0];
    color_tc = color_v[0];
    distance_tc = distance_v[0];
    depth_tc = z;
    chance_tc = chance_v[0];
}
