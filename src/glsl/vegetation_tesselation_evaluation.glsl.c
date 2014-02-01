layout(isolines, point_mode, equal_spacing) in;

in vec3 position_tc[];
patch in float distance_tc;
patch in vec3 apex_tc, left_tc, right_tc,  stratum_tc;
patch in vec4 color_tc;
patch in int index_tc;

out vec3 position_te, normal_te;
out vec4 color_te;
out float distance_te;
out int index_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
vec2 hash(uvec2 U, unsigned int k);
vec4 rand4();
              
uniform grass_debug{
    int debug;
};

uniform float clustering;
                                
void main()
{
    vec2 u;
    float sqrtux;

    u = hash(floatBitsToUint(position_tc[0].xy),
             floatBitsToUint(gl_TessCoord.x));
    u = (u + stratum_tc.xy) / stratum_tc.z;

    sqrtux = sqrt(u.x);

    position_te = ((1 - sqrtux) * apex_tc +
                   sqrtux * (1 - u.y) * left_tc +
                   sqrtux * u.y * right_tc);
    
    normal_te = normalize(cross(left_tc - apex_tc, right_tc - apex_tc));
    
    color_te = color_tc;
    distance_te = distance_tc;
    index_te = index_tc;
}
