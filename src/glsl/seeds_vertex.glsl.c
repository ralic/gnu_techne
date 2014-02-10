in vec3 apex, left, right, stratum;
in vec4 color;
in float distance;
in float clustering;
in uvec2 chance;

out vec3 position_v, normal_v;
out vec4 color_v;

void main()
{
    vec2 u;
    float sqrtux;
    
    u = (rand2(chance) + stratum.xy) / stratum.z;
    sqrtux = sqrt(u.x);
    
    position_v = ((1 - sqrtux) * apex +
                  sqrtux * (1 - u.y) * left +
                  sqrtux * u.y * right);
    
    normal_v = normalize(cross(left - apex, right - apex));
    color_v = color;
}
