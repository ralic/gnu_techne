layout(points) in;
layout(points, max_vertices = 1) out;
              
in vec3 position_te[], normal_te[];
in vec4 color_te[];
in float distance_te[];
in int index_te[];

out vec4 color_g;
out vec3 normal_g, position_g;
flat out float distance_g;              
              
uniform grass_debug{
    int debug;
};

void main()
{
    const vec3 l = -normalize(vec3(1, 1, 1));
    const vec2 horizon = vec2(100, 1.0 / 3.0);

    mat4 T;
    vec4 h, p_0, p_1, p;
    float z, z_f, n, ldotn;

    color_g = color_te[0];    
    distance_g = distance_te[0];
    normal_g = normal_te[0];
    position_g = position_te[0];
    /* EmitStreamVertex(index_te[0]); */
    EmitVertex();

    /* EndStreamPrimitive(index_te[0]);         */
    EndPrimitive();        
}
