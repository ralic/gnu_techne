out vec4 fragment;
in vec3 eye;
in vec2 uv;

uniform sampler2D base, detail[N];
uniform float power;
uniform vec3 references[N], weights[N];
uniform vec2 resolutions[N];

uniform float turbidity, factor, beta_p;
uniform vec3 direction, intensity, beta_r;

void main()
{
    vec3 tau, pigments[N], sum, hsv;
    float C, cosine, phase_r, phase_p, rho, distances[N];
    int i;

    hsv = vec3(texture2D(base, uv));

    /* Calculate the distance to each pigment. */
    
    for (i = 0, C = 0.0 ; i < N ; i += 1) {
        distances[i] = 1 / pow(hsv_distance (hsv, references[i], weights[i]), power);
        
        C += distances[i];
    }

    for (i = 0, sum = vec3(0.0) ; i < N ; i += 1) {
        sum += distances[i] / C *
            texture2D(detail[i], uv / resolutions[i]).rgb;
    }

    /* sum = texture2D(detail[2], uv / resolutions[2]).rgb; */
    /* Calculate aerial perspective. */
    
    cosine = dot(normalize(eye), direction);
    phase_r = 0.059683 * (1.0 + cosine * cosine);
    tau = exp(beta_r * eye.z);
    rho = exp((0.6544 * turbidity - 0.6510) * beta_p * eye.z);

    /* Mix the final fragment color. */
    
    fragment = vec4(intensity * mix(50.0 * vec3(phase_r),
                                    factor * hsv.z * sum,
                                    tau), rho);
    
    /* fragment = fragment * 1e-9 + vec4(factor * vec3(texture2D(base, uv)), 1.0); */
    /* fragment = vec4(vec3(texture2D(base, uv)), 1.0); */
}
