in vec3 apex, left, right;

out vec3 position, color;
out float distance;
flat out int index, instance;
flat out vec3 apex_v, left_v, right_v, st_v;

uniform sampler2D base, detail[N];
uniform vec3 intensity, references[N], weights[N];
uniform vec2 resolutions[N];
uniform float factor;

uniform vec2 offset, scale;

float hsv_distance (vec3, vec3, vec3, float);
vec2 hash(unsigned int R, unsigned int L, unsigned int k);

uniform float power;

uniform grass_debug{
    int debug;
};

#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 0) uniform atomic_uint infertile;
layout(binding = 0, offset = 4) uniform atomic_uint drawn;
#endif

void main()
{
    vec3 hsv, c, s, t, r_0;
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
    hsv = vec3(texture2D(base, uv));
    
    for (i = 0, d_0 = d_1 = -infinity, i_0 = -1 ; i < N ; i += 1) {
        float d;
        
        d = 1 / pow(hsv_distance (hsv, references[i], weights[i]), power);
        
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

    apex_v = apex; left_v = left; right_v = right;
    st_v = vec3(a, l);
}
