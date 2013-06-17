in vec3 color_g, normal_g;
in float height_g;
out vec4 fragment;

void main()
{
    fragment = vec4(color_g * (0.65 * max(height_g, 1) + 0.45 * abs(dot(mat3(modelview) * normalize(vec3(1, 1, 1)), normalize(normal_g))))/* * 1e-9 + vec3(1) */, 1);
    /* fragment = vec4(vec3(height_g), 1); */
}
