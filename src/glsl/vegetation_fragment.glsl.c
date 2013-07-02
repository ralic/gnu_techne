in vec3 color_g, normal_g;
in vec2 uv_g;
flat in int index_g;
out vec4 fragment;

uniform sampler2D masks[N];

void main()
{
    vec4 texel;

    texel = texture2D(masks[index_g], uv_g);

    if (texel.a > 0) {
        fragment = vec4(texel.rgb/* color_g */ * (0.65 * max(uv_g.x, 1) + 0.45 * abs(dot(mat3(modelview) * normalize(vec3(1, 1, 1)), normalize(normal_g))))/* * 1e-9 + vec3(1) */, 1);
        /* fragment = vec4(vec3(height_g), 1); */
    } else {
        discard;
    }
}
