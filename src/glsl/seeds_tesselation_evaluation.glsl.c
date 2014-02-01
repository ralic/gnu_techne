layout(isolines, equal_spacing) in;

in vec3 position_tc[];
patch in vec4 color_tc;
patch in vec3 normal_tc;

out vec4 color_te;
                                
#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 8) uniform atomic_uint segments;
#endif
                                
uniform seeds_evaluation {
    float height;
};

void main()
{
    /* Update the statistics. */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(segments);
#endif

    gl_Position = transform * vec4(position_tc[0] + height * gl_TessCoord.x * normal_tc, 1);
    color_te = color_tc;
}
