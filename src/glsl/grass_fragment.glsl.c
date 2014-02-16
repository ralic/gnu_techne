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
    vec2 ambient, specular;
    vec3 diffuse;
};

void main()
{
    vec4 texel;

    texel = texture(mask, uv_g);
    
    /* fragment = vec4(uv_g, 1, 1); return; */
    /* fragment = texel.rgba; return; */

    if (distance_g < 0) {
        fragment = color_g;
    } else if (texel.a > 0) {
        vec3 L_e = direction;
        vec3 n = (gl_FrontFacing ? -1 : 1) * normalize(normal_g);
        vec3 R = reflect(L_e, n);
        float A, D_r, D_s, S, tau, nL_e;

        nL_e = dot(n, L_e);

        A = ambient[0] * mix(uv_g.y, 1, ambient[1]);

        tau = 1 / abs(nL_e);
        D_r = diffuse[0] * max(nL_e, A);
        D_s = diffuse[1] * max(-nL_e, A) * exp(-diffuse[2] * tau);

        S = specular[0] * pow(max(-R.z, 0), specular[1]);
            
        fragment = vec4(mix(texel.rgb + intensity * color_g.rgb * (D_r + mix(vec3(1), vec3(1, 1, 0.6), abs(nL_e)) * D_s), intensity, S), texel.a * color_g.a);
        /* fragment = vec4(vec3(1, 0, 0) * D_s + vec3(0, 1, 0) * D_r, 1); */
    } else {
        discard;
    }
}
