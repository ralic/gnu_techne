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

    color_g = color_te[0];

    if (category_te[0] == 0) {
        const float sqrt_2 = sqrt(2);
        vec4 b_0, b_1;
        float rho = 0.01;

        b_0 = rho * vec4(plane_te[0][0], 0);
        b_1 = rho * vec4(plane_te[0][1], 0);

        /* A stem. */

        gl_Position = PM * (position_te[0] + b_0);
        EmitVertex();

        gl_Position = PM * (position_te[1] + b_0);
        EmitVertex();

        gl_Position = PM * (position_te[0] + b_1);
        EmitVertex();

        gl_Position = PM * (position_te[1] + b_1);
        EmitVertex();

        gl_Position = PM * (position_te[0] - b_0);
        EmitVertex();

        gl_Position = PM * (position_te[1] - b_0);
        EmitVertex();

        gl_Position = PM * (position_te[0] - b_1);
        EmitVertex();

        gl_Position = PM * (position_te[1] - b_1);
        EmitVertex();

        gl_Position = PM * (position_te[0] + b_0);
        EmitVertex();

        gl_Position = PM * (position_te[1] + b_0);
        EmitVertex();
    } else {
        vec4 h = 0.03 * vec4(plane_te[0][0], 0);
    
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
