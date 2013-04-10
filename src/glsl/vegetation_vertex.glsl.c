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
void srand(vec2 seed);
vec2 rand(void);

void main()
{
    vec3 texel, hsv, c, s, t;
    vec2 uv, u;
    float d_0, d_1, r, sqrtux;
    int i, i_0;

    srand(left.xy * gl_InstanceID);
    u = rand();
    sqrtux = sqrt(u.x);
    
    c = (1 - sqrtux) * apex +
        sqrtux * (1 - u.y) * left +
        sqrtux * u.y * right;

    /* ... */

    uv = scale * c.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, d_0 = d_1 = 1.0 / 0.0, i_0 = -1 ; i < N ; i += 1) {
        float d;
        
        d = hsv_distance (hsv, references[i], weights[i]);
        
        if (d < d_0) {
            d_1 = d_0;
            d_0 = d;
            i_0 = i;
        }
    }
    
    /* ... */
    
    position = c;
    chance = rand();
    index = i_0;
    distance = 1 - d_0 / d_1;
    color = factor * intensity * (texel.r + texel.g + texel.b) / 3.0 * texture2D(detail[i_0], uv / resolutions[i_0]).rgb;
}
