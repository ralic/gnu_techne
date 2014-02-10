in vec3 apex, left, right, stratum;
in vec4 color;
in float distance;
in float clustering;
in uvec2 chance;

out vec3 apex_v, left_v, right_v, stratum_v;
out vec4 color_v;
out float distance_v;
out float clustering_v;
out uvec2 chance_v;

void main()
{
    apex_v = apex;
    left_v = left;
    right_v = right;
    stratum_v = stratum;
    color_v = color;
    distance_v = distance;
    clustering_v = clustering;
    chance_v = chance;
}
