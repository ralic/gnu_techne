void Grass_geometry()
{
    mat4 PM = projection * modelview;

    gl_Position = PM * (gl_in[0].gl_Position + vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (gl_in[0].gl_Position - vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (gl_in[1].gl_Position + vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_Position = PM * (gl_in[1].gl_Position - vec4(0, 0.005, 0, 0));
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
