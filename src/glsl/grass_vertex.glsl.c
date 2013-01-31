in vec4 positions;

void main()
{
    gl_Position = projection * modelview * positions;
}
