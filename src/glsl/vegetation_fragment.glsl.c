in vec3 color_g;
out vec4 fragment;

void main()
{
    fragment = vec4(color_g * 1e-9 + vec3(1), 1);
}
