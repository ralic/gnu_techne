layout(vertices = 1) out;

in vec3 apex_v[], left_v[], right_v[], stratum_v[], normal_v[];
in vec4 color_v[];
in float clustering_v[];
in unsigned int instance_v[];

patch out vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc;
patch out vec4 color_tc;
patch out unsigned int instance_tc;

void main() {
    gl_TessLevelOuter[0] = clustering_v[0];
    gl_TessLevelOuter[1] = 1;
    
    apex_tc = apex_v[0];
    left_tc = left_v[0];
    right_tc = right_v[0];
    stratum_tc = stratum_v[0];
    color_tc = color_v[0];
    normal_tc = normal_v[0];
    instance_tc = instance_v[0];
}
