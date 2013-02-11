layout(points) in;
/* layout(points, max_vertices = 50) out; */
layout(line_strip, max_vertices = 100) out;
   
uniform vec3 colors[N];
               
uniform grass_debug {               
    bool debug;
};
               
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
    
    if (n.z < 1.0) {
        s = normalize(vec3(n.y, -n.x, 0));
        t = cross (s, n);
    } else {
        s = vec3(1, 0, 0);
        t = vec3(0, 1, 0);
    }

    next = floatBitsToUint (c.xy);

    for (i = 0 ; i < N ; i += 1) {
        for (j = 0 ; j < cluster[0].counts[i] ; j += 1) {
            vec4 c_0;
            vec2 z;

            z = rand();
            c_0 = vec4(c + r * z.x * s + r * z.y * t, 1);
            
            color = colors[i];
            gl_Position = mvp * c_0;
            gl_PrimitiveID = gl_PrimitiveIDIn;
            EmitVertex();

            z = (z + 1) * vec2(3.1415926, 0.3 * 3.1415926 / 2.0);
            gl_Position = mvp * (c_0 + 0.02 * vec4(cos(z.x) * sin(z.y),
                                                   sin(z.x) * sin(z.y),
                                                   cos(z.y), 0));
            gl_PrimitiveID = gl_PrimitiveIDIn;
            EmitVertex();

            EndPrimitive();
        }
    }
}
