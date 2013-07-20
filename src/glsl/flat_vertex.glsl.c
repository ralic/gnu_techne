uniform flat_vertex {
    vec4 color;
};

in vec4 positions;
out vec4 colors;

void main()
{
    gl_Position = transform * positions;
    colors = color;
}
