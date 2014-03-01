in vec3 apex, left, right;

out vec3 color_v;
out float distance_v;
out int index_v;
out vec3 apex_v, left_v, right_v;
out unsigned int instance_v;

uniform float factor, thresholds[SWATCHES];
uniform vec2 offset, scale;

vec2 hash(unsigned int R, unsigned int L, unsigned int k);
vec2 rand2();
uvec2 srand();

vec3 compose(const vec2 uv);
float splat_distance(const vec3 hsv, const int i, const int j);

vec3 cluster_center(vec3 apex, vec3 left, vec3 right, unsigned int instance);

uniform float instances;

uniform vegetation_debug{
    int debug;
    bool debugtoggle;
};

#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 0) uniform atomic_uint infertile;
layout(binding = 0, offset = 4) uniform atomic_uint drawn;
#endif

void main()
{
    vec3 rgb, hsv, c;
    vec2 uv;
    float d_0, d_1, r;
    int i, i_0, i_1;

    c = cluster_center(apex, left, right, uint(gl_InstanceID));
    
    /* Find the seed type. */
    
    uv = fma(scale, c.xy, offset);
    rgb = compose(uv);
    hsv = rgb_to_hsv(rgb);
    
    for (i = 0, d_0 = d_1 = -infinity, i_0 = i_1 = -1 ; i < SWATCHES ; i += 1) {
        float d;
        
        d = splat_distance(hsv, i, 0);
        
        if (d > d_0) {
            d_1 = d_0;
            i_1 = i_0;

            d_0 = d;
            i_0 = i;
        } else if (d > d_1) {
            d_1 = d;
            i_1 = i;
        }
    }
    
    vec2 z = rand2();
    float z_0 = d_1 / (d_0 + d_1);
    
    if (/* debug > 0 &&  */z.x < z_0 && d_0 < thresholds[i_0]) {
        if (d_1 < thresholds[i_1]) {
#ifdef COLLECT_STATISTICS
            atomicCounterIncrement(infertile);
#endif

            index_v = -1;
            return;
        }

        index_v = i_1;
        distance_v = d_1 / d_0;
    } else {
        /* Skip the rest if the seed is infertile. */
    
        if (d_0 < thresholds[i_0]) {
#ifdef COLLECT_STATISTICS
            atomicCounterIncrement(infertile);
#endif

            index_v = -1;
            return;
        }

        index_v = i_0;
        distance_v = 1 - d_1 / d_0;
    }
    /* index_v = 0; distance_v = 1; color_v = vec4(1, 0, 0, 1); */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(drawn);
#endif
        
    color_v = factor * rgb;
    apex_v = apex; left_v = left; right_v = right;
    instance_v = uint(gl_InstanceID);
}
