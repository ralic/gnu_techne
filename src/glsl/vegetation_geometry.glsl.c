layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
              
flat in int index_te[];

in vec4 position_te[];
in vec3 color_te[];
in float distance_te[], height_te[];
in mat2x3 plane_te[];
flat in float depth_te[];
out vec3 color_g, normal_g;
out float height_g;
              
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
    
    if (debug < 0)
        h = ((1 / z) * PM + (1 - 1 / z) * projection * pow(z, -0.1 * debug)) * h;
    else
        h = PM * h * pow(z, 0.1 * debug);

    /* A flat leaf. */

    normal_g = mat3(modelview) * plane_te[0][0];
    height_g = height_te[0];
    
    gl_Position = PM * position_te[0] + h;
    EmitVertex();

    gl_Position = PM * position_te[0] - h;
    EmitVertex();

    normal_g = mat3(modelview) * plane_te[1][0];
    height_g = height_te[1];
    
    gl_Position = PM * position_te[1] + h;
    EmitVertex();

    gl_Position = PM * position_te[1] - h;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
