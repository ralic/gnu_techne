in vec3 color_g, normal_g;
in vec2 uv_g;
in float foo_g;
flat in int index_g, shadow_g;
flat in float distance_g;              

out vec4 fragment;

uniform sampler2D masks[N];
              
uniform grass_debug{
    int debug;
};

void main()
{
    vec4 texel;

    if (debug > 0 && shadow_g == 1) {
        texel = texture(masks[index_g], uv_g, foo_g * 10 * float(debug));
    } else {
        texel = texture(masks[index_g], uv_g);
    }
    
    if (texel.g > 0) {
        if (shadow_g == 1) {
            fragment = vec4(0, 0, 0, texel.g);
        } else{
            fragment = vec4(texel.r * color_g * ((0.25 * pow(abs((fract(uv_g.x) * 2) - 1), 2) + 1.65 * clamp(uv_g.y, 1 - distance_g, 1.0) + 0.5 * abs(dot(mat3(modelview) * normalize(vec3(1, 1, 1)), normalize(normal_g)))))/* * 1e-9 + vec3(1) */, texel.g);
            /* fragment = vec4(texel.g); */
        }
    } else {
        discard;
    }
}
