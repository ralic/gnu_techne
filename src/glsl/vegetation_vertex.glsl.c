in vec3 apex, left, right;

out vec3 position, color;
flat out vec2 chance;
out float distance;
flat out int index;

uniform sampler2D base, detail[N];
uniform vec3 intensity, references[N], weights[N];
uniform vec2 resolutions[N];
uniform float factor;

uniform float scale;
uniform vec2 offset;

vec3 rgb_to_hsv (vec3);
float hsv_distance (vec3, vec3, vec3, float);
vec2 hash(unsigned int R, unsigned int L, unsigned int k);
void srand(uvec2 seed);
vec2 rand(void);

uniform grass_debug{
    int debug;
};

void main()
{
    vec3 texel, hsv, c, s, t;
    vec2 uv, u;
    float dd, d_0, d_1, r, sqrtux;
    int i, i_0, i_1;

    vec2 center;
    const float phi = (sqrt(5) - 1.0) / 2.0;
    
    center = (apex.xy + left.xy + right.xy) / 3;

#if 0
    {
        int i, a, b, c, p;

        i = gl_InstanceID + 1;
        a = 2 * int(round(center.x)) + 1;
        b = 2 * int(round(center.y));
        p = (a * i + b * i * i) % 8192;//16384;
        srand(uvec2(4294967295.0 * normalize(center) * fract(phi * p)));
        u = rand();
    }
#else
    {
        u = hash(floatBitsToUint(center.x), floatBitsToUint(center.y),
                 gl_InstanceID);
    }
#endif
    
    sqrtux = sqrt(u.x);
    
    c = (1 - sqrtux) * apex +
        sqrtux * (1 - u.y) * left +
        sqrtux * u.y * right;

    /* ... */

    uv = scale * c.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, d_0 = d_1 = -1.0 / 0.0, i_0 = i_1 = -1 ; i < N ; i += 1) {
        float d;
        
        d = 1 / pow(hsv_distance (hsv, references[i], weights[i]), 2);
        
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
    
    /* ... */

    /* if (debug) { */
        dd = d_1 / d_0;

        if (dd > 0.75) {
            u = rand();

            if (u.x > 0.75 * dd) {
                i = i_0;
                r = 1 - dd;
            } else {
                i = i_1;
                r = dd;
            }
        } else {
            i = i_0;
            r = 1 - dd;
        }
    /* } else { */
    /*     i = i_0; */
    /* } */
        /* i = 2; */
        
    position = c;
    chance = rand();
    index = i;
    distance = r;
    color = /* vec3(1, 0, 0) + 1e-9 *  */factor * intensity * (texel.r + texel.g + texel.b) / 3.0 * texture2D(detail[i], uv / resolutions[i]).rgb;
}
