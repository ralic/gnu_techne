layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
              
flat in int index_te[];

in vec4 position_te[];
in vec3 color_te[];
in float distance_te[];
in mat2x3 plane_te[];
flat in int category_te[];             
flat in float depth_te[];
out vec3 color_g;

uniform grass_debug{
    int debug;
};

void main()
{
    mat4 PM, T;
    vec4 h;
    float z;
    
    color_g = color_te[0];

    z = depth_te[0];
    h = 0.001 * distance_te[0] * vec4(plane_te[0][1], 0);
    PM = projection * modelview;
    
#if 0
    h = ((1 / z) * PM + (1 - 1 / z) * projection) * h;
#else
    h = PM * h * pow(z, 1.0 / (2.0 + debug));
#endif

    /* A flat leaf. */
    
    gl_Position = PM * position_te[0] + h;
    EmitVertex();

    gl_Position = PM * position_te[0] - h;
    EmitVertex();

    gl_Position = PM * position_te[1] + h;
    EmitVertex();

    gl_Position = PM * position_te[1] - h;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
