layout(points) in;
layout(points, max_vertices = 5) out;

uniform vec3 colors[N];
               
in cluster_attributes {
    vec3 center, normal;
    float size;
    int counts[N];
} cluster[1];

out vec3 color;
               
void main()
{
    mat4 mvp;
    vec3 n, s, t;
    int i, j;
    
    float rand_s[5] = float[5](-1, 1, 0.0, 0, 0);
    float rand_t[5] = float[5](0, 0, 0.0, 1, -1);
    int i_s = 0, i_t = 0;
    
    mvp = projection * modelview;
    s = vec3(cluster[0].normal.y,
             -cluster[0].normal.x,
             0);
    t = cross (s, cluster[0].normal);

    for (i = 0 ; i < N ; i += 1) {
        for (j = 0 ; j < cluster[0].counts[i] ; j += 1) {
            color = colors[i];
            gl_Position = mvp * (vec4(cluster[0].center + 0.2 * rand_s[i_s++] * s + 0.2 * rand_t[i_t++] * t, 1));
            gl_PointSize = cluster[0].size;
            gl_PrimitiveID = gl_PrimitiveIDIn;

            EmitVertex();
            EndPrimitive();
        }
    }
}
