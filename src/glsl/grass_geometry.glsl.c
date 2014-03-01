#define CAST_SHADOWS

#ifdef CAST_SHADOWS
layout(lines, invocations = 2) in;
#else
layout(lines) in;
#endif
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 position_te[], tangent_te[], bitangent_te[], color_te[];
in float distance_te[], height_te[], depth_te[], width_te[];
in vec4 plane_te[];

out vec4 position_g;
out vec3 normal_g;
out vec2 uv_g;
flat out vec3 color_g;
flat out float distance_g;              

uniform vec3 direction_w;

void project_shadow(vec4 plane, vec3 direction, inout mat4 modelview);

void main()
{
    mat4 M, P;
    mat3 R;
    vec4 h, p_0, p_1, p;
    
    P = projection;
    M = modelview;
    R = mat3(modelview);
    
    color_g = color_te[0];
    
#ifdef CAST_SHADOWS
    if (gl_InvocationID == 0) {
#endif
        distance_g = distance_te[0];

#ifdef CAST_SHADOWS
    } else {
        project_shadow(plane_te[0], direction_w, M);
        distance_g = -1;
    }
#endif
    
    p_0 = vec4(position_te[0], 1);
    p_1 = vec4(position_te[1], 1);    
    h = 0.5 * width_te[0] * vec4(bitangent_te[0], 0);
    
    normal_g = R * tangent_te[0];
    
    uv_g = vec2(1, height_te[0]);
    position_g = M * (p_0 + h);
    gl_Position = P * position_g;
    EmitVertex();

    uv_g = vec2(0, height_te[0]);
    position_g = M * (p_0 - h);
    gl_Position = P * position_g;
    EmitVertex();

    normal_g = R * tangent_te[1];
    
    uv_g = vec2(1, height_te[1]);
    position_g = M * (p_1 + h);
    gl_Position = P * position_g;
    EmitVertex();

    uv_g = vec2(0, height_te[1]);
    position_g = M * (p_1 - h);
    gl_Position = P * position_g;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();        
}
