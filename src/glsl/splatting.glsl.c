uniform sampler2D base, detail[3 * N];
uniform vec2 resolutions[3 * N];
uniform vec3 references[N], weights[N];
uniform float separation;

uniform splatingDebug {
    int splatlevel, splatdebug;
    float splatmin, splatmax;
};

float splat_distance(const vec3 hsv, const int i, const int j)
{
    return pow(hsv_distance (hsv, references[i], weights[i]), -separation);
}

vec3 compose(const vec2 uv)
{
    vec3 S, B;
    float S_0;
    int i, j;

    B = vec3(texture2D(base, uv));
    
    /* Calculate the distance to each pigment. */

    for (j = 0, S = B ; j < splatlevel ; j += 1) {
        vec3 H, L;
        float C;

        H = rgb_to_hsv(S);
        
        for (i = 0, L = vec3(0), C = 0 ; i < N ; i += 1) {
            float D, l;
            vec3 T;

            D = splat_distance(H, i, j);            
            T = vec3(texture2D(detail[3 * i + j], uv / resolutions[3 * i + j]));

            C += D;
            L += D * T;
        }

        if (j < 2)
        S_0 = length(S);
        S += L / C;
    }

    /* return S; */
    /* return pow((S - 0.5) * splatmax * length(B) + 0.5, vec3(1 / splatmin)); */
    /* return log(S - splatmin) / log(splatmax - splatmin); */
    /* return pow((S - splatmin), vec3(splatmax)) / pow(3, splatmax); */
    float C = 0.5 + abs(0.5 - (B.r + B.b + B.g) / 3);
    return pow(1 / (1 + exp(-(S - 0.5) * splatmax * C)), vec3(1 / splatmin));
}
