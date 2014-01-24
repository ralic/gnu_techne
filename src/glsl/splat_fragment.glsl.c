out vec4 fragment;
in vec3 eye;
in vec2 uv;

uniform float turbidity, factor, beta_p;
uniform vec3 direction, intensity, beta_r;

void main()
{
    vec3 tau, rgb;
    float cosine, phase_r, phase_p, rho;

    rgb = compose(uv);
    
    /* Calculate aerial perspective. */
    
    cosine = dot(normalize(eye), direction);
    phase_r = 0.059683 * (1.0 + cosine * cosine);
    tau = exp(beta_r * eye.z);
    rho = exp((0.6544 * turbidity - 0.6510) * beta_p * eye.z);

    /* Mix the final fragment color. */
    
    fragment = vec4(intensity * mix(50.0 * vec3(phase_r),
                                    factor * rgb,
                                    0*tau + 1), 0*rho+1);

    /* fragment = vec4(vec3(max(0, dot(L, normal))), 1); */
    
    /* fragment = fragment * 1e-9 + vec4(vec3(rgb.z), 1.0); */
}
