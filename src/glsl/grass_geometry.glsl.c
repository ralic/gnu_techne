void Grass_geometry()
{
    mat4 PM = projection * modelview;

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
