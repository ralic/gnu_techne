layout(points) in;
layout(points, max_vertices = 5) out;

uniform vec3 colors[N];
               
in int counts[1][N];
out vec3 color;
               
void main()
{
    int i, j;

    for (i = 0 ; i < N ; i += 1) {
        for (j = 0 ; j < counts[0][i] ; j += 1) {
            gl_Position = gl_in[0].gl_Position + vec4(i * 1, j * 1, 0, 0);
            gl_PointSize = gl_in[0].gl_PointSize;
            gl_PrimitiveID = gl_PrimitiveIDIn;
            color = colors[i];

            EmitVertex();
            EndPrimitive();
        }
    }
}
