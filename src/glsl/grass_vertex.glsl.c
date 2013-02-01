in vec4 positions;

void main()
{
    gl_Position = projection * modelview * gl_Vertex;
    gl_PointSize = 1;
}
