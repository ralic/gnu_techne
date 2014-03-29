in vec4 position_g;
in vec3 normal_g;
in vec2 uv_g;
in float height_g;
flat in float distance_g;              
flat in vec3 color_g;

out vec4 fragment;

uniform sampler2D mask;
uniform vec3 intensity, direction;
              
uniform grass_debug{
    float debug;
    bool debugtoggle;
};

uniform grass_fragment{
    vec2 ambient, specular, diffuse;
    float attenuation;
};

void main()
{
    vec4 texel;

    texel = texture(mask, uv_g, 0);

    if (distance_g < 0) {
        fragment = vec4(0, 0, 0, 1 - ambient[0]) * texel;
    } else if (texel.a > 0) {
        vec3 l = direction;
        vec3 n = normalize(normal_g);
        vec3 v = normalize(-position_g.xyz);
        float A, D_0, D, T, S, nl;

        n *= sign(dot(n, v));            
        nl = dot(n, l);

        A = ambient[0] * mix(ambient[1], 1, height_g);
        D_0 = diffuse[0] * mix(diffuse[1], 1, height_g);
        D = D_0 * max(nl, 0);
        T = D_0 * exp(-attenuation / abs(nl)) * max(-nl, 0);

        /* fragment = vec4(vec3(D + T), 1); */
            
        S = specular[0] * pow(max(dot(reflect(-l, n), v), 0), specular[1]);

        fragment = vec4(intensity * (texel.rgb + color_g * (A + D + vec3(1, 1, 0.9) * T) + S), texel.a);
    } else {
        discard;
    }
}
