in vec3 apex, left, right, color;
in float distance;
in float clustering;
in unsigned int instance;

out vec3 apex_v, left_v, right_v, stratum_v, normal_v, color_v;
out float clustering_v;
out unsigned int instance_v;

void main()
{
    vec2 u, p;
    float l;

    apex_v = apex;
    left_v = left;
    right_v = right;
    color_v = color;
    clustering_v = clustering;
    instance_v = instance;
    normal_v = normalize(cross(left - apex, right - apex));

    p = (apex.xy + left.xy + right.xy) / 3;

    u = hash(floatBitsToUint(p.xy), uint(instance));
    l = ceil(pow(3 * float(instance + 1), 1.0 / 3.0) - 0.5);
    stratum_v = vec3(floor(rand2() * l), l);
}
