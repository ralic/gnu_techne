in vec3 apex, left, right, color;
in float score;
in float clustering;
in unsigned int instance;

out vec3 apex_v, left_v, right_v, normal_v, color_v;
out float score_v;
out float clustering_v;
out unsigned int instance_v;

void main()
{
    apex_v = apex;
    left_v = left;
    right_v = right;
    color_v = color;
    score_v = score;
    clustering_v = clustering;
    instance_v = instance;
    normal_v = normalize(cross(left - apex, right - apex));
}
