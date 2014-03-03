in vec3 normal_g;
in vec2 uv_g;
flat in vec4 color_g;
out vec4 fragment;

uniform vec3 intensity, direction;

uniform sampler2D palette;

uniform gravel_fragment{
    float ambient, diffuse;
};

void main()
{
    float nl = dot(normalize(normal_g), direction);
    vec3 color = texture(palette, uv_g).rgb;

    fragment = vec4(intensity * color * (ambient + diffuse * max(nl, 0)), 1) * color_g;
}
