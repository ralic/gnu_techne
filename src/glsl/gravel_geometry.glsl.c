#define CAST_SHADOWS

#ifdef CAST_SHADOWS
layout(lines, invocations = 2) in;
#else
layout(lines) in;
#endif
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 normal_te[], position_te[], color_te[];
in vec4 plane_te[], chance_te[];

out vec4 position_g;
out vec3 normal_g;
out vec2 uv_g;
flat out vec4 color_g;

uniform vec3 direction_w;

void main()
{
    mat4 M, P;
    mat3 R;
    
    P = projection;
    M = modelview;
    R = mat3(modelview);
    
#ifdef CAST_SHADOWS
    if (gl_InvocationID == 0) {
#endif
        color_g = vec4(color_te[0], 1);

#ifdef CAST_SHADOWS
    } else {
        mat4 P;
        vec4 l;

        l = vec4(direction_w / dot(direction_w, plane_te[0].xyz), 0);
        M *= mat4(1.0) - outerProduct(l, plane_te[0]);
        
        color_g = vec4(0, 0, 0, 0.6);
    }
#endif

    const vec3 rho = vec3(1e-3, 1e-3, 1e-3) + vec3(2e-3, 2e-3, 1e-3) * chance_te[0].xyz;
    const vec3 delta = vec3(0, 0, rho.z);
    
    uv_g = vec2(0, 0);
    normal_g = R * vec3(0, 0, 1);
    position_g = M * vec4(position_te[0] + rho * vec3(0, 0, 1) + delta, 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    uv_g = vec2(0, 0);
    normal_g = R * normal_te[0];
    position_g = M * vec4(position_te[0] + rho * normal_te[0] + delta, 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    uv_g = vec2(0, 0);
    normal_g = R * normal_te[1];
    position_g = M * vec4(position_te[1] + rho * normal_te[1] + delta, 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    uv_g = vec2(0, 0);
    normal_g = R * vec3(0, 0, -1);
    position_g = M * vec4(position_te[1] + rho * vec3(0, 0, -1) + delta, 1);
    gl_Position = P * position_g;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();        
}
