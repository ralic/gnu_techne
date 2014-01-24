in vec4 color_g;
in vec3 normal_g;
in vec2 uv_g;
flat in int index_g;
flat in float distance_g;              

out vec4 fragment;

uniform sampler2D masks[N];
              
uniform grass_debug{
    int debug;
    bool debugtoggle;
};

void main()
{
    vec4 texel;

    texel = texture(masks[0], uv_g);
    
    /* fragment = vec4(uv_g, 1, 1); */

    if (texel.a > 0) {
        const vec3 L = normalize(vec3(1, 0, 1));
        vec3 L_e = mat3(modelview) * L;
        vec3 n = normalize(normal_g);
        vec3 R = reflect(L_e, n);
        float A_0, A, D_r, D_s, S;

        A_0 = clamp(uv_g.y, 1.1 - distance_g, 1.0);
        A = 0.85;
        D_r = 0.65 * abs(dot(L_e, n));
        D_s = 2 * abs(-dot(L_e, n)) * exp(-float(debug) * clamp(dot(n, -L_e), 0.3 * (debugtoggle ? pow(1 - 2 * abs(0.5 - uv_g.x), 2) : 1), 1.0));
        S = 0.2 * pow(clamp(R.z, 0, 1), 10);
            
        fragment = vec4(texel.rgb + color_g.rgb * A_0 * (A + D_r + D_s) + S * vec3(1)/* * 1e-9 + vec3(1) */, texel.a * color_g.a);
    } else {
        discard;
    }
}
