in vec3 positions;
in vec3 normals;
in float sizes;

out seed_attributes {
    vec3 position, color;
    int index;
} seed;

uniform sampler2D base, foo;
uniform vec3 references[N], weights[N];
uniform float power;

uniform float scale;
uniform vec2 offset;

vec3 rgb_to_hsv (vec3);
float hsv_distance (vec3, vec3, vec3, float);
void srand(uvec2 seed);
vec2 rand(void);

void main()
{
    vec3 texel, hsv, c, n, s, t;
    vec2 uv;
    vec2 u;
    float D, r;
    int i, argmin;

    /* ... */

    n = normals;
    if (any(greaterThan(n.xy, vec2(0)))) {
        s = normalize(vec3(n.y, -n.x, 0));
        t = cross (s, n);
    } else {
        s = vec3(1, 0, 0);
        t = vec3(0, 1, 0);
    }

    srand(floatBitsToUint (positions.xy * (gl_InstanceID + 1)));
    
    r = sizes;
    u = rand();
    c = positions + r * u.x * s + r * u.y * t;

    /* ... */

    uv = scale * c.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, D = 0, argmin = -1 ; i < N ; i += 1) {
        float d;
        
        d = hsv_distance (hsv, references[i], weights[i], power);

        if (d > D) {
            D = d;
            argmin = i;
        }
    }
    
    /* ... */
    
    seed.position = c;
    seed.index = argmin;
    seed.color = 2 * (texel.r + texel.g + texel.b) / 3.0 * vec3(0.742141, 0.681327, 0.588593) * texture2D(foo, uv / 0.0025).rgb;
}
