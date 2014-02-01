in vec3 position, normal;
in vec4 color;
in float distance;

out vec3 position_v, normal_v;
out vec4 color_v;

void main()
{
    position_v = position;
    normal_v = normal;
    color_v = color;
}
