layout(vertices = 1) out;

in vec3 apex_v[], left_v[], right_v[], color_v[];
in float clustering_v[];
in unsigned int instance_v[];

patch out vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc;
patch out unsigned int instance_tc;


uniform gravel_control {
    float detail, amplification;
};

void main() {
    const float bias = 1;
    
    vec4 p_e;
    vec3 p;
    float z, n;
    
    p = (apex_v[0] + left_v[0] + right_v[0]) / 3;
    p_e = modelview * vec4(p, 1);
    z = max(-p_e.z, bias);
    n = max(3, bias * detail / z / z);

    gl_TessLevelOuter[0] = clustering_v[0] * amplification / z / z;
    gl_TessLevelOuter[1] = n;
    
    apex_tc = apex_v[0];
    left_tc = left_v[0];
    right_tc = right_v[0];
    color_tc = color_v[0];
    instance_tc = instance_v[0];
    normal_tc = normalize(cross(left_v[0] - apex_v[0],
                                right_v[0] - apex_v[0]));
    stratum_tc = cluster_stratum(apex_v[0], left_v[0], right_v[0],
                                 instance_v[0]);    
}