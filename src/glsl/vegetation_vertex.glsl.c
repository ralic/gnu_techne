in vec3 apex, left, right;

out vec3 position;
out vec4 color;
out float distance;
flat out int index, instance;
flat out vec3 apex_v, left_v, right_v, stratum_v;

uniform vec3 intensity;
uniform float factor;

uniform vec2 offset, scale;

float hsv_distance (vec3, vec3, vec3, float);
vec2 hash(unsigned int R, unsigned int L, unsigned int k);

uniform float instances;

uniform grass_debug{
    int debug;
};

#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 0) uniform atomic_uint infertile;
layout(binding = 0, offset = 4) uniform atomic_uint drawn;
#endif

void main()
{
    vec3 rgb, hsv, c, s, t, r_0;
    vec2 uv, u, a;
    float d_0, d_1, r, sqrtux, l;
    int i, i_0, i_1;

    vec2 center;

    center = (apex.xy + left.xy + right.xy) / 3;
    u = hash(floatBitsToUint(center.xy), uint(gl_InstanceID));

    if (true) {
        float S;

        S = float(gl_InstanceID + 1);
        l = ceil(pow(3 * S, 1.0 / 3.0) - 0.5);
    } else {
        int S;
        
        for (i = 0, S = 1 ; S <= gl_InstanceID + 1 ; i += 1, S += i * i);
        l = float(i);
        /* float S; */

        /* S = float(gl_InstanceID + 1); */
        /* l = pow(2, ceil(log(1 + 3 * S) / log(4)) - 1); */
    }

    a = floor(rand2() * l);
    u = (u + a) / l;    
    sqrtux = sqrt(u.x);
    
    c = (1 - sqrtux) * apex +
        sqrtux * (1 - u.y) * left +
        sqrtux * u.y * right;
    
    /* Find the seed type. */
    
    uv = fma(scale, c.xy, offset);
    rgb = compose(uv);
    hsv = rgb_to_hsv(rgb);
    
    for (i = 0, d_0 = d_1 = -infinity, i_0 = -1 ; i < N ; i += 1) {
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
    
    if (/* debug > 0 &&  */z.x < z_0 && i_0 > 0) {
        if (i_1 > 0 || d_0 < 1000) {
#ifdef COLLECT_STATISTICS
            atomicCounterIncrement(infertile);
#endif

            index = -1;
            return;
        }

        i = i_1;
        r = max(0, d_1 / d_0 - 1);
    } else {
        /* Skip the rest if the seed is infertile. */
    
        if (i_0 > 0 || d_0 < 1000) {
#ifdef COLLECT_STATISTICS
            atomicCounterIncrement(infertile);
#endif

            index = -1;
            return;
        }

        i = i_0;
        r = 1 - d_1 / d_0;
    }
    /* i = 0; r = 1; */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(drawn);
#endif
        
    position = c;
    index = i;
    instance = gl_InstanceID;
    distance = r;
    const float transition = 0.3;
    color = vec4(factor * intensity * rgb,
                 min(1, 1 / transition - gl_InstanceID / (transition * instances)));

    apex_v = apex; left_v = left; right_v = right;
    stratum_v = vec3(a, l);
}
