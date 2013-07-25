in vec3 apex, left, right;

out vec3 position, color ;
out float distance;
flat out int index, instance;
flat out vec3 apex_v, left_v, right_v;

uniform sampler2D base, detail[N];
uniform vec3 intensity, references[N], weights[N];
uniform vec2 resolutions[N];
uniform float factor;

uniform vec2 offset, scale;

float hsv_distance (vec3, vec3, vec3, float);
vec2 hash(unsigned int R, unsigned int L, unsigned int k);

uniform vec3 clustering;
uniform grass_debug{
    int debug;
};

#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 0) uniform atomic_uint infertile;
layout(binding = 0, offset = 4) uniform atomic_uint drawn;
#endif

void main()
{
    vec4 c_4;
    vec3 hsv, c, s, t, r_0;
    vec2 uv, u;
    float d_0, d_1, r, sqrtux;
    int i, i_0, i_1;

    vec2 center;
    
    center = (apex.xy + left.xy + right.xy) / 3;

    u = hash(floatBitsToUint(center.xy), uint(gl_InstanceID));
    
    sqrtux = sqrt(u.x);
    
    c = (1 - sqrtux) * apex +
        sqrtux * (1 - u.y) * left +
        sqrtux * u.y * right;

    /* Test the seed point against the frustum. */

    c_4 = vec4(c, 1);
    
    /* Find the seed type. */
    
    uv = fma(scale, c.xy, offset);
    hsv = vec3(texture2D(base, uv));
    
    for (i = 0, d_0 = d_1 = -infinity, i_0 = -1 ; i < N ; i += 1) {
        float d;
        
        d = 1 / pow(hsv_distance (hsv, references[i], weights[i]), 5);
        
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

    /* Skip the rest if the seed is infertile. */
    
    if (i_0 != 0) {
#ifdef COLLECT_STATISTICS
        atomicCounterIncrement(infertile);
#endif

        index = -1;
        return;
    }

    i = i_0;
    r = 1 - d_1 / d_0;

    /* i = 0; r = 1; */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(drawn);
#endif
        
    position = c;
    index = i;
    instance = gl_InstanceID;
    distance = r;
    color = /* vec3(1, 0, 0) + 1e-9 *  */factor * intensity * hsv.z * texture2D(detail[i], uv / resolutions[i]).rgb;

    /* if (clustering[0] > 1) { */
    /*     color = vec3(1, 0, 0); */
    /* } */

    apex_v = apex; left_v = left; right_v = right;
}
