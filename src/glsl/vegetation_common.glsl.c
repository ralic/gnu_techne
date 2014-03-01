vec2 rand2();
vec2 hash(uvec2 U, unsigned int k);

void seed_cluster(vec3 apex, vec3 left, vec3 right,
                  unsigned int instance,
                  out vec2 u, out vec2 a, out float l)
{
    vec2 p;

    p = (apex.xy + left.xy + right.xy) / 3;

    u = hash(floatBitsToUint(p.xy), instance);
    l = ceil(pow(3 * float(instance + 1), 1.0 / 3.0) - 0.5);
    a = floor(rand2() * l);
}

vec3 cluster_stratum(vec3 apex, vec3 left, vec3 right, unsigned int instance)
{
    vec2 u, a;
    float l;
    
    seed_cluster(apex, left, right, instance, u, a, l);
    
    return vec3(a, l);
}

vec3 cluster_center(vec3 apex, vec3 left, vec3 right, unsigned int instance)
{
    vec2 u, v, a;
    float l;

    seed_cluster(apex, left, right, instance, u, a, l);
    u = (u + a) / l;    

    if (false) {
        float sqrtux = sqrt(u.x);
    
        return (1 - sqrtux) * apex +
            sqrtux * (1 - u.y) * left +
            sqrtux * u.y * right;
    } else {
        v = mix(u, 1 - u.yx, floor(u.x + u.y));
        return apex + (left - apex) * v.x + (right - apex) * v.y;
    }
}

vec3 cluster_seed(vec3 apex, vec3 left, vec3 right, vec3 stratum,
                  unsigned int instance)
{
    vec2 u;

    /* Hash the triangle vertices and instance to get a pair of random
     * numbers. */
    
    u = hash(floatBitsToUint((apex.xy + left.xy + right.xy) / 3),
             floatBitsToUint(gl_TessCoord.y) + instance);
    u = (u + stratum.xy) / stratum.z;

    /* Map the random numbers onto the triangle (0, 0) - (1, 0) -
       (0, 1) by reflecting the points lying outside it along the edge
       (1, 0) - (0, 1).*/
    
    u = mix(u, 1 - u.yx, floor(u.x + u.y));
    return apex + (left - apex) * u.x + (right - apex) * u.y;
}

void project_shadow(vec4 plane, vec3 direction, inout mat4 modelview)
{
    vec4 l;

    l = vec4(direction / dot(direction, plane.xyz), 0);
    modelview *= mat4(1.0) - outerProduct(l, plane);
}
