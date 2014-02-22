#define CAST_SHADOWS

#ifdef CAST_SHADOWS
layout(lines, invocations = 2) in;
#else
layout(lines) in;
#endif
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 position_te[], tangent_te[], bitangent_te[];
in float distance_te[], height_te[], depth_te[], width_te[];
in vec4 plane_te[], color_te[];

out vec4 color_g, position_g;
out vec3 normal_g;
out vec2 uv_g;
flat out float distance_g;              

uniform vec3 direction_w;

void main()
{
    const vec3 l = direction_w;
    const vec2 horizon = vec2(100, 1.0 / 3.0);

    mat4 M, P;
    mat3 R;
    vec4 h, p_0, p_1, p;
    float z, z_f, n;
    
    z_f = 5 + 10 / horizon[1] * (depth_te[0] / horizon[0] - 1);
    n = exp(3e-4 * depth_te[0] * depth_te[0] * (1 / (1 + exp(z_f))));    
    h = 0.5 * n * width_te[0] * vec4(bitangent_te[0], 0);

    P = projection;
    M = modelview;
    R = mat3(modelview);
    
#ifdef CAST_SHADOWS
    if (gl_InvocationID == 0) {
#endif
        color_g = color_te[0];    
        distance_g = distance_te[0];

#ifdef CAST_SHADOWS
    } else {
        mat4 P;
        vec4 L;

        L = vec4(l / dot(l, plane_te[0].xyz), 0);
        M *= mat4(1.0) - outerProduct(L, plane_te[0]);
        
        color_g = vec4(0, 0, 0, 0.6);
        distance_g = -1;
    }
#endif
    
    p_0 = vec4(position_te[0], 1);
    p_1 = vec4(position_te[1], 1);    
    
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
