layout(lines) in;

in vec4 position_te[];
in vec3 color_te[];
in float distance_te[];
in int category_te[];             
out vec3 color_g;

void Grass_geometry()
{
    mat4 PM = projection * modelview;

    color_g = color_te[0];

    if (category_te[0] == 0) {
        const float sqrt_2 = sqrt(2);
        float rho = 0.001;

        /* A stem. */

        gl_Position = PM * (position_te[0] + rho * vec4(-sqrt_2, -sqrt_2, 0, 0));
        EmitVertex();

        gl_Position = PM * (position_te[1] + rho * vec4(-sqrt_2, -sqrt_2, 0, 0));
        EmitVertex();

        gl_Position = PM * (position_te[0] + rho * vec4(-sqrt_2, sqrt_2, 0, 0));
        EmitVertex();

        gl_Position = PM * (position_te[1] + rho * vec4(-sqrt_2, sqrt_2, 0, 0));
        EmitVertex();

        gl_Position = PM * (position_te[0] + rho * vec4(0, 1, 0, 0));
        EmitVertex();

        gl_Position = PM * (position_te[1] + rho * vec4(0, 1, 0, 0));
        EmitVertex();
    } else {
        vec4 h = vec4(0, distance_te[0] * 0.03, 0, 0);
    
        /* A flat leaf. */
    
        gl_Position = PM * (position_te[0] + h);
        EmitVertex();

        gl_Position = PM * (position_te[0] - h);
        EmitVertex();

        gl_Position = PM * (position_te[1] + h);
        EmitVertex();

        gl_Position = PM * (position_te[1] - h);
        EmitVertex();
    }

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
