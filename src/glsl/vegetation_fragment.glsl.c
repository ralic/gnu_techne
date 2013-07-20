in vec3 color_g, normal_g;
in vec2 uv_g;
flat in int index_g;
flat in float distance_g;              

out vec4 fragment;

uniform sampler2D masks[N];

void main()
{
    vec4 texel;

    texel = texture2D(masks[index_g], uv_g);

    if (texel.g > 0) {
        fragment = vec4((texel.r * color_g * (1.65 * clamp(uv_g.y, 1 - distance_g, 1.0) + 0.5 * abs(dot(mat3(modelview) * normalize(vec3(1, 1, 1)), normalize(normal_g)))))/* * 1e-9 + vec3(1) */, texel.g);
        /* fragment = vec4(1 - distance_g); */
    } else {
        discard;
    }

    /* fragment = vec4(vec3(uv_g.y), 1); */
}
