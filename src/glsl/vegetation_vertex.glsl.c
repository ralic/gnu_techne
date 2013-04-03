in vec3 positions;
in vec3 normals;
in float sizes;

out vec3 position, color;
flat out int index;

uniform sampler2D base, detail[N];
uniform vec3 intensity, references[N], weights[N];
uniform vec2 resolutions[N];
uniform float power, factor;

uniform float scale;
uniform vec2 offset;

vec3 rgb_to_hsv (vec3);
float hsv_distance (vec3, vec3, vec3, float);
void srand(uvec2 seed);
vec2 rand(void);

void main()
{
    vec3 texel, hsv, c, n, s, t;
    vec2 uv, u;
    float D, r;
    int i, j;

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
    
    for (i = 0, D = 0, j = -1 ; i < N ; i += 1) {
        float d;
        
        d = hsv_distance (hsv, references[i], weights[i], power);

        if (d > D) {
            D = d;
            j = i;
        }
    }
    
    /* ... */
    
    position = c;
    index = j;
    color = factor * intensity * (texel.r + texel.g + texel.b) / 3.0 * texture2D(detail[j], uv / resolutions[j]).rgb;
}
