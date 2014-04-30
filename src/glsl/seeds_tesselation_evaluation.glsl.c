layout(isolines, equal_spacing) in;

patch in vec3 apex_tc, left_tc, right_tc, stratum_tc, normal_tc, color_tc,
    position_tc;
patch in float clustering_tc;
patch in unsigned int instance_tc;

flat out vec3 color_te;

uniform seeds_evaluation {
    float height;
};

void main()
{
    vec3 p;

    /* Update the statistics. */

    if (clustering_tc > 1) {
        p = cluster_seed(apex_tc, left_tc, right_tc, stratum_tc, instance_tc);
    } else {
        p = position_tc;
    }

    gl_Position = transform * vec4(p + height * gl_TessCoord.x * normal_tc, 1);
    color_te = color_tc;
}
