#define CAST_SHADOWS

#ifdef CAST_SHADOWS
layout(lines, invocations = 2) in;
#else
layout(lines) in;
#endif
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 position_te[], color_te[], tangent_te[], bitangent_te[];
              in float distance_te[], height_te[], depth_te[], foo_te[];
in vec4 plane_te[];
in int index_te[];

out vec3 color_g, normal_g;
out vec2 uv_g;
out float foo_g;              
flat out int index_g, shadow_g;
flat out float distance_g;              
              
uniform grass_debug{
    int debug;
};

void main()
{
    const vec3 l = -normalize(vec3(1, 1, 1));

    mat4 T;
    vec4 h, p_0, p_1, p;
    float z, n, ldotn;
    
    p_0 = vec4(position_te[0], 1);
    p_1 = vec4(position_te[1], 1);
    
    z = depth_te[0];
    n = floor(pow(z, 0.75));
    h = 0.5 * n * 0.0035 * distance_te[0] * vec4(bitangent_te[0], 0);

    /* A flat leaf. */

    index_g = index_te[0];
    shadow_g = gl_InvocationID;
    
#ifdef CAST_SHADOWS
    if (gl_InvocationID == 0) {
#endif
        color_g = color_te[0];    
        distance_g = distance_te[0];
        normal_g = mat3(modelview) * tangent_te[0];

        T = transform;
#ifdef CAST_SHADOWS
    } else {
        mat4 P;
        vec4 L;

        L = vec4(l / dot(l, plane_te[0].xyz), 0);
        P = mat4(1.0) - outerProduct(L, plane_te[0]);
        T = transform * P;
        
        color_g = vec3(0);
        distance_g = 0;
        normal_g = vec3(0);
    }
#endif

    ldotn = dot(l, plane_te[0].xyz);
    
    p = p_0 + h;
    foo_g = abs((dot(p.xyz, plane_te[0].xyz) + plane_te[0].w) / ldotn);
    uv_g = vec2(n, height_te[0]);
    gl_Position = T * p;
    EmitVertex();

    p = p_0 - h;
    foo_g = abs((dot(p.xyz, plane_te[0].xyz) + plane_te[0].w) / ldotn);
    uv_g = vec2(0, height_te[0]);
    gl_Position = T * p;
    EmitVertex();

    p = p_1 + h;
    foo_g = abs((dot(p.xyz, plane_te[0].xyz) + plane_te[0].w) / ldotn);
    uv_g = vec2(n, height_te[1]);
    gl_Position = T * p;
    EmitVertex();

    p = p_1 - h;
    foo_g = abs((dot(p.xyz, plane_te[0].xyz) + plane_te[0].w) / ldotn);
    uv_g = vec2(0, height_te[1]);
    gl_Position = T * p;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();        
}
