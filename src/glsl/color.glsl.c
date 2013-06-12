float hsv_distance (vec3 tuple, vec3 reference, vec3 weights)
{
    vec3 v;

    v = abs(tuple - reference);
    v.x = min(v.x, 1.0 - v.x);
    v *= weights;

    return dot(v, v);
}
