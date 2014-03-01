in vec3 normal_g;
flat in vec4 color_g;
out vec4 fragment;

uniform vec3 intensity, direction;

uniform gravel_fragment{
    float ambient, diffuse;
};

void main()
{
    float nl = dot(normalize(normal_g), direction);

    fragment = vec4(intensity * (ambient + diffuse * max(nl, 0)), 1) * color_g;
}
