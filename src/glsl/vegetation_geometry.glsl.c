layout(points) in;
layout(points, max_vertices = 1) out;
              
in vec4 color_v[];
in float distance_v[];
in int index_v[];
in vec3 apex_v[], left_v[], right_v[], stratum_v[];
in uvec2 chance_v[];

out vec4 color_g;
out vec3 apex_g, left_g, right_g, stratum_g;
out float distance_g;              
out uvec2 chance_g;
out unsigned int clustering_g;              

uniform float clustering;

uniform grass_debug{
    int debug;
};

void main()
{
    if (index_v[0] < 0) {
        return;
    }
    
    color_g = color_v[0];    
    distance_g = distance_v[0];
    apex_g = apex_v[0];
    left_g = left_v[0];
    right_g = right_v[0];
    stratum_g = stratum_v[0];
    clustering_g = uint(clustering);
    chance_g = chance_v[0];
    /* EmitStreamVertex(index_te[0]); */
    EmitVertex();

    /* EndStreamPrimitive(index_te[0]);         */
    EndPrimitive();        
}
