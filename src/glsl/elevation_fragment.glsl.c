out vec4 fragment;
in vec3 eye;
in vec2 uv;

uniform sampler2D base, detail[N];
uniform float power;
uniform mat3 matrices[N];
/* uniform float turbidity, factor, beta_p; */
/* uniform vec3 sunDirection, sunColor, beta_r; */

void main()
{
    vec3 texel, tau, pigments[N], sum, hsv;
    float cosine, phase_r, phase_p, rho, H, M, m, C, distances[N];
    int i;

    texel = vec3(texture2D(base, uv));

    M = max(max (texel.r, texel.g), texel.b);
    m = min(min (texel.r, texel.g), texel.b);
    C = M - m;

    if (C > 0.0) {
        if (texel.r == M) {
            H = mod((texel.g - texel.b) / C, 6.0) / 6.0;
        } else if (texel.g == M) {
            H = ((texel.b - texel.r) / C + 2.0) / 6.0;
        } else {
            H = ((texel.r - texel.g) / C + 4.0) / 6.0;
        }
    } else {
        H = 0.0;
    }

    hsv = vec3(H, C / M, M);

    for (i = 0, C = 0.0 ; i < N ; i += 1) {
        vec3 v;
        v = hsv - matrices[i][0];
        v.x = min(v.x, 1.0 - v.x);
        v *= matrices[i][1];
        distances[i] = 1.0 / pow(dot(v, v), power);
        C += distances[i];
    }

    for (i = 0, sum = vec3(0.0) ; i < N ; i += 1) {
             sum += distances[i] / C *
                       texture2D(detail[i], matrices[i][2].st * uv).rgb;
    }

    /* cosine = dot(normalize(eye), sunDirection); */
    /* phase_r = 0.059683 * (1.0 + cosine * cosine); */
    /* tau = exp(beta_r * eye.z); */
    /* rho = exp((0.6544 * turbidity - 0.6510) * beta_p * eye.z); */

    /* texel = (texel.r + texel.g + texel.b) / 3.0 * sum; */

    /* fragment = vec4(sunColor * mix(50.0 * vec3(phase_r), */
    /*                                    factor * texel, */
    /*                                    tau), rho); */

    fragment = vec4(vec3(sum), 1.0);
    //fragment = vec4(vec3(C), 1.0);
}
