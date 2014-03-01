layout(vertices = 1) out;

in vec3 apex_v[], left_v[], right_v[], normal_v[], color_v[];
in float clustering_v[];
in unsigned int instance_v[];

patch out vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc,
    position_tc;
patch out float clustering_tc;
patch out unsigned int instance_tc;

void main() {
    gl_TessLevelOuter[0] = clustering_v[0];
    gl_TessLevelOuter[1] = 1;
    
    apex_tc = apex_v[0];
    left_tc = left_v[0];
    right_tc = right_v[0];
    color_tc = color_v[0];
    normal_tc = normal_v[0];
    instance_tc = instance_v[0];
    clustering_tc = clustering_v[0];

    if (clustering_v[0] > 1) {
        stratum_tc = cluster_stratum(apex_v[0], left_v[0], right_v[0],
                                     instance_v[0]);
    } else {
        position_tc = cluster_center(apex_v[0], left_v[0], right_v[0],
                                     instance_v[0]);
    }
}
