layout(points, invocations = 32) in;
layout(triangle_strip, max_vertices = 15) out;

uniform sampler2D base, foo;
uniform vec3 references[N], weights[N];
uniform float power;

uniform float scale;
uniform vec2 offset;

uniform public_attributes {
    int amplification;
};
               
in cluster_attributes {
    vec3 center, normal;
    float size;
} cluster[1];

out vec4 color;

vec3 rgb_to_hsv (vec3);
float hsv_distance (vec3, vec3, vec3, float);
void srand(uvec2 seed);

void main()
{
    vec3 texel, hsv, n, s, t, c, p;
    vec2 uv;
    float D, r;
    int i, j, argmin;

    r = cluster[0].size;
    c = cluster[0].center;
    n = cluster[0].normal;

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
    
    if (n.z < 1.0) {
        s = normalize(vec3(n.y, -n.x, 0));
        t = cross (s, n);
    } else {
        s = vec3(1, 0, 0);
        t = vec3(0, 1, 0);
    }

    srand(floatBitsToUint (c.xy) + gl_InvocationID);

#define EXPAND_CLUSTER(FUNCTION, i)                                     \
    if (i == argmin) {                                                  \
        color = 2 * (texel.r + texel.g + texel.b) / 3.0 * vec4(vec3(0.742141, 0.681327, 0.588593) * texture2D(foo, uv / 0.0025).rgb, 1.0); \
                                                                        \
        FUNCTION(modelview, projection, c, n, s, t, r, 1);   \
    }

        /* Continued in vegetation_geometry_botom.glsl.c */
