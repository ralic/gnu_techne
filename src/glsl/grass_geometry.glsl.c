layout(points) in;
layout(points, max_vertices = 5) out;

uniform vec3 colors[N];
               
in cluster_attributes {
    vec3 center, normal;
    float size;
    int counts[N];
} cluster[1];

out vec3 color;

uvec2 next;

vec2 rand(void)
{
    const uvec2 a = uvec2(1664525u, 1013904223u);
    const uvec2 c = uvec2(22695477u, 1u);
    next = next * a + c;
    return vec2 (next) / 2147483648.0 - 1.0;
}

void main()
{
    mat4 mvp;
    vec3 n, s, t, c;
    float r;
    int i, j;
    
    mvp = projection * modelview;
    r = cluster[0].size;
    c = cluster[0].center;
    n = cluster[0].normal;
    s = vec3(n.y, -n.x, 0);
    t = cross (s, n);

    next = floatBitsToUint (c.xy);
    
    for (i = 0 ; i < N ; i += 1) {
        for (j = 0 ; j < cluster[0].counts[i] ; j += 1) {
            vec2 z;

            z = rand();
            
            color = colors[i];
            gl_Position = mvp * (vec4(c + r * z.x * s + r * z.y * t, 1));
            gl_PrimitiveID = gl_PrimitiveIDIn;

            EmitVertex();
            EndPrimitive();
        }
    }
}
