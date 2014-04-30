vec3 rgb_to_hsv (vec3 rgb)
{
    float H, M, m, C;

    M = max(max (rgb.r, rgb.g), rgb.b);
    m = min(min (rgb.r, rgb.g), rgb.b);
    C = M - m;

    if (C > 0.0) {
        if (rgb.r == M) {
            H = mod((rgb.g - rgb.b) / C, 6.0) / 6.0;
        } else if (rgb.g == M) {
            H = ((rgb.b - rgb.r) / C + 2.0) / 6.0;
        } else {
            H = ((rgb.r - rgb.g) / C + 4.0) / 6.0;
        }
    } else {
        H = 0.0;
    }

    return vec3(H, C / M, M);
}

float hsv_distance (vec3 tuple, vec3 reference, vec3 weights)
{
    vec3 v;

    v = abs(tuple - reference);
    v.x = min(v.x, 1.0 - v.x);
    v *= weights;

    return dot(v, v);
}
