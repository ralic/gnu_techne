layout(lines) in;

in vec4 position_te[];
in vec3 color_te[];
out vec3 color_g;

void Grass_geometry()
{
    mat4 PM = projection * modelview;

    color_g = color_te[0];
    
    gl_Position = PM * (position_te[0] + vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (position_te[0] - vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (position_te[1] + vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (position_te[1] - vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
