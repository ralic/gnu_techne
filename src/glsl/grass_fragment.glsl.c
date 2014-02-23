in vec4 position_g;
in vec3 normal_g;
in vec2 uv_g;
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
    vec2 ambient, specular;
    vec3 diffuse;
};

void main()
{
    vec4 texel;

    texel = texture(mask, uv_g);

    if (distance_g < 0) {
        fragment = vec4(0, 0, 0, 1 - ambient[0]) * texel;
    } else if (texel.a > 0) {
        vec3 l = direction;
        vec3 n = (gl_FrontFacing ? -1 : 1) * normalize(normal_g);
        vec3 v = normalize(-position_g.xyz);
        float A, D, T, S, nl;

        nl = dot(n, l);
        A = ambient[0] * mix(uv_g.y, 1, ambient[1]);
        D = diffuse[0] * max(nl, 0);

        if (nl < 0) {
            T = diffuse[0] * -nl * exp(diffuse[1] / nl);
        } else {
            T = 0;
        }
        
        S = specular[0] * pow(max(dot(reflect(l, n), v), 0), specular[1]);
            
        fragment = vec4(texel.rgb + intensity * (color_g * (A + D + vec3(1, 1, 0.6) * T) + S), texel.a);
    } else {
        discard;
    }
}
