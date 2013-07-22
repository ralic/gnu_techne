layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 position_te[];
in vec3 color_te[];
in float distance_te[], height_te[];
in mat2x3 plane_te[];
flat in float depth_te[];
flat in int index_te[];

out vec3 color_g, normal_g;
out vec2 uv_g;
flat out int index_g;
flat out float distance_g;              
              
uniform grass_debug{
    int debug;
};

void main()
{
    vec4 h, p_0, p_1;
    float z, n;
    
    color_g = color_te[0];
    
    p_0 = vec4(position_te[0], 1);
    p_1 = vec4(position_te[1], 1);
    
    z = depth_te[0];
    n = floor(pow(z, 0.75));
    h = 0.5 * n * 0.0035 * distance_te[0] * vec4(plane_te[0][1], 0);

    /* A flat leaf. */

    index_g = index_te[0];
    distance_g = distance_te[0];
    normal_g = mat3(modelview) * plane_te[0][0];
    
    uv_g = vec2(n, height_te[0]);
    gl_Position = transform * (p_0 + h);
    EmitVertex();

    uv_g = vec2(0, height_te[0]);
    gl_Position = transform * (p_0 - h);
    EmitVertex();

    normal_g = mat3(modelview) * plane_te[1][0];
    
    uv_g = vec2(n, height_te[1]);
    gl_Position = transform * (p_1 + h);
    EmitVertex();

    uv_g = vec2(0, height_te[1]);
    gl_Position = transform * (p_1 - h);
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
