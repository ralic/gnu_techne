layout(lines) in;

in vec4 position_te[];
in vec3 color_te[];
in float distance_te[];
out vec3 color_g;

void Grass_geometry()
{
    mat4 PM = projection * modelview;
    vec4 h = vec4(0, distance_te[0] * 0.003, 0, 0);

    color_g = color_te[0];
    
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
