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

void project_shadow(vec4 plane, vec3 direction, inout mat4 modelview);

void main()
{
    mat4 M, P;
    mat3 R;
    vec3 rho, p;
    float w, h;
    
    P = projection;
    M = modelview;
    R = mat3(modelview);
    
#ifdef CAST_SHADOWS
    if (gl_InvocationID == 0) {
#endif
        color_g = vec4(color_te[0], 1);

#ifdef CAST_SHADOWS
    } else {
        project_shadow(plane_te[0], direction_w, M);
        color_g = vec4(0, 0, 0, 0.6);
    }
#endif
    
    w = 1e-3 + 1e-3 * chance_te[0].x;
    h = (0.25 + 0.35 * chance_te[0].z) * w;

    /* ***** */

    float theta = 2 * pi * chance_te[0].w;
    float phi = atan(h / w);

    mat3 T = mat3(vec3(1, 0, 0),
                  vec3(0, cos(phi), sin(phi)),
                  vec3(0, -sin(phi), cos(phi))) *
             mat3(vec3(cos(theta), sin(theta), 0),
                  vec3(-sin(theta), cos(theta), 0),
                  vec3(0, 0, 1));    

    /* ***** */

    p = position_te[0] + vec3(0, 0, h);
    rho = vec3(w, (1 + 0.35 * chance_te[0].y) * w, h);
    
    uv_g = chance_te[0].yw;
    normal_g = R * T * vec3(0, 0, 1);
    position_g = M * vec4(p + T * (rho * vec3(0, 0, 1)), 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    normal_g = R * T * normal_te[0];
    position_g = M * vec4(p + T * (rho * normal_te[0]), 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    normal_g = R * T * normal_te[1];
    position_g = M * vec4(p + T * (rho * normal_te[1]), 1);
    gl_Position = P * position_g;
    EmitVertex();
    
    normal_g = R * T * vec3(0, 0, -1);
    position_g = M * vec4(p + T * (rho * vec3(0, 0, -1)), 1);
    gl_Position = P * position_g;
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();        
}
