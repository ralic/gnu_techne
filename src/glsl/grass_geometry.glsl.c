layout(lines) in;

in vec4 position_te[];
in vec3 color_te[];
in float distance_te[];
in mat2x3 plane_te[];
flat in int category_te[];             
out vec3 color_g;

void Grass_geometry()
{
    mat4 PM = projection * modelview;
    vec4 h = 0.004 * vec4(plane_te[0][1], 0);

    color_g = color_te[0];

    /* A flat leaf. */
    
    gl_Position = PM * (position_te[0] + h);
    EmitVertex();

    gl_Position = PM * (position_te[0] - h);
    EmitVertex();

    gl_Position = PM * (position_te[1] + h);
    EmitVertex();

    gl_Position = PM * (position_te[1] - h);
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
