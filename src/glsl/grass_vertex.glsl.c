layout(location=0) in vec4 positions;

void main()
{
    gl_Position = projection * modelview * positions;
    gl_PointSize = 1;
}
