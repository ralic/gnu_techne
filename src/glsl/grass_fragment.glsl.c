in vec4 color_g;
in vec3 normal_g;
in vec2 uv_g;
flat in float distance_g;              

out vec4 fragment;

uniform sampler2D mask;
uniform vec3 intensity, direction;
              
uniform grass_debug{
    float debug;
    bool debugtoggle;
};

uniform grass_fragment{
    float ambient;
    vec2 specular;
    vec3 diffuse;
};

void main()
{
    vec4 texel;

    texel = texture(mask, uv_g);
    
    /* fragment = vec4(uv_g, 1, 1); return; */
    /* fragment = texel.rgba; return; */

    if (texel.a > 0) {
        vec3 L_e = direction;
        vec3 n = (gl_FrontFacing ? -1 : 1) * normalize(normal_g);
        vec3 R = reflect(L_e, n);
        float A_0, A, D_r, D_s, S, tau, nL_e;

        nL_e = dot(n, L_e);

        A = ambient * clamp(uv_g.y, 1 - distance_g, 1.0);
        D_r = diffuse[0] * max(nL_e, 0);

        tau = 1 - abs(nL_e);
        D_s = diffuse[1] * max(-nL_e, 0) * exp(-diffuse[2] * tau);

        S = specular[0] * pow(abs(R.z), specular[1]);
            
        fragment = vec4(texel.rgb + intensity * (color_g.rgb * (A + D_r) + mix(color_g.rgb, vec3(1), 1) * D_s + S), min(1, texel.a * color_g.a));
    } else {
        discard;
    }
}
