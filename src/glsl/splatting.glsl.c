<@begin main @>
const int SWATCHES = <@= context.swatches_n @>;

<@ if context.bands_n > 0 then @>
const int MAX_BANDS = <@= context.bands_max @>;
const int TOTAL_BANDS = <@= context.bands_n @>;
const int BANDS[2 * <@= context.swatches_n @>] = {
    <@ for i = 1, context.swatches_n do @>
    <@= context.bands[i][1] @>, <@= context.bands[i][2] @>,
    <@ end @>
};
uniform sampler2D detail[TOTAL_BANDS];
uniform vec2 resolutions[TOTAL_BANDS];
<@ end @>

uniform sampler2D base;
uniform vec3 references[SWATCHES], weights[SWATCHES];
uniform float separation;

uniform splatingDebug {
    int splatdebug;
    float splatmin, splatmax;
};

float hsv_distance (vec3 tuple, vec3 reference, vec3 weights);
vec3 rgb_to_hsv (vec3 rgb);

float splat_score(const vec3 hsv, const int i, const int j)
{
    return pow(hsv_distance (hsv, references[i], weights[i]), -separation);
}

vec3 compose(const vec2 uv)
{
    vec3 S, B;
    int i, j;

    B = vec3(texture2D(base, uv));

<@ if context.bands_n == 0 then @>
    return B;
<@ else @>

    /* Calculate the distance to each pigment. */

    for (j = 0, S = B ; j < MAX_BANDS ; j += 1) {
        vec3 H, L;
        float C;

        H = rgb_to_hsv(S);

        for (i = 0, L = vec3(0), C = 0 ; i < SWATCHES ; i += 1) {
            float D, l;
            vec3 T;
            int k;

            if (j >= BANDS[2 * i + 1]) {
                continue;
            }

            k = BANDS[2 * i] + j;

            D = splat_score(H, i, j);
            T = vec3(texture2D(detail[k], uv / resolutions[k]));

            C += D;
            L += D * T;
        }

        S += L / C;
    }

    /* return S; */
    /* return pow((S - 0.5) * splatmax * length(B) + 0.5, vec3(1 / splatmin)); */
    /* return log(S - splatmin) / log(splatmax - splatmin); */
    /* return pow((S - splatmin), vec3(splatmax)) / pow(3, splatmax); */
    float C = 0.5 + abs(0.5 - (B.r + B.b + B.g) / 3);
    return pow(1 / (1 + exp(-(S - 0.5) * splatmax * C)), vec3(1 / splatmin));
<@ end @>
}

<@finish @>
