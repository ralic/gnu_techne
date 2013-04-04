in vec3 apex, left, right;

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
    vec3 texel, hsv, c, s, t;
    vec2 uv, u;
    float D, r, sqrtux;
    int i, j;

    srand(floatBitsToUint (left.xy * (gl_InstanceID + 1)));
    
    u = rand();
    sqrtux = sqrt(u.x);
    
    c = (1 - sqrtux) * apex +
        sqrtux * (1 - u.y) * left +
        sqrtux * u.y * right;

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
