layout(vertices = 1) out;

in vec3 position_v[];
in vec4 color_v[];
in float distance_v[];
flat in int index_v[];
flat in vec3 apex_v[], left_v[], right_v[], stratum_v[];

out vec3 position_tc[];
patch out vec3 apex_tc, left_tc, right_tc, stratum_tc;
patch out vec4 color_tc;
patch out int index_tc;
patch out float distance_tc;

uniform float clustering;

void main() {
    int i;
    
    i = index_v[0];

    if (i < 0) {
        gl_TessLevelOuter[0] = 0;
        gl_TessLevelOuter[1] = 0;

        return;
    }
    
    gl_TessLevelOuter[0] = 1;
    gl_TessLevelOuter[1] = clustering - 1;
    
    color_tc = color_v[0];    
    position_tc[gl_InvocationID] = position_v[0];
    index_tc = i;
    distance_tc = distance_v[0];

    apex_tc = apex_v[0]; left_tc = left_v[0]; right_tc = right_v[0];
    stratum_tc = stratum_v[0]; 
}
