in vec4 positions;
in vec2 mapping;
out vec2 uv;

void main()
{
    gl_Position = transform * positions;
    uv = mapping;
}
